#include "common.h"
#include "preServer.h"
#include "BaseLog.h"
#include "Config.h"

DH *dh512;
DH *dh1024;

void handle_error(const char *file, int lineno, const char *msg)
{
    LOGPRINT(DEBUG, "** %s:%i %s. errno is %d\n", file, lineno, msg, errno);
    ERR_print_errors_fp(stderr);
    exit(-1);
}

void init_OpenSSL(void)
{
    if (!THREAD_setup() || ! SSL_library_init()) //初始化
    {
        LOGPRINT(DEBUG, "** OpenSSL initialization failed!\n");
        exit(-1);
    }
    SSL_load_error_strings();  // 打印调试信息
}

int verify_callback(int ok, X509_STORE_CTX *store)
{
    if (!ok)
    {
        char data[256];
        X509 *cert = X509_STORE_CTX_get_current_cert(store);
        int depth = X509_STORE_CTX_get_error_depth(store);
        int err = X509_STORE_CTX_get_error(store);
        LOGPRINT(DEBUG, "-Error with certificate at depth: %i\n", depth);
        X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);   // 证书签署者(CA)的名字
        LOGPRINT(DEBUG, " issuer = %s\n", data);
        X509_NAME_oneline(X509_get_subject_name(cert), data, 256);  // 证书所有者
        LOGPRINT(DEBUG, " subject = %s\n", data);
        LOGPRINT(DEBUG, " err %i:%s\n", err, X509_verify_cert_error_string(err));
    }
    return ok;
}

long post_connection_check(CINEMACONNECT *arg)
{
    CINEMACONNECT *ca = static_cast<CINEMACONNECT *>(arg);
    X509 *cert;
    X509_NAME *subj;
    char data[256];
    int extcount;
    int ok = 0;
    EVP_PKEY* evpPkey;
    struct rsa_st *RsaSt;
    unsigned char RsaN[200] = {0};
    unsigned char RsaE[200] = {0};
    /* Checking the return from SSL_get_peer_certificate here is not
    * strictly necessary. With our example programs, it is not
    * possible for it to return NULL. However, it is good form to
    * check the return since it can return NULL if the examples are
    * modified to enable anonymous ciphers or for the server to not
    * require a client certificate.
    */
    if (!(cert = SSL_get_peer_certificate(ca->ssl)))   // 从SSL结构中提取出对方的证书
        goto err_occured;
    if ((extcount = X509_get_ext_count(cert)) > 0)
    {
        int i;
        for (i = 0; i < extcount; i++)
        {
            LOGPRINT(DEBUG, "certificate verification 11\n");
#if 0
            const char *extstr;
            X509_EXTENSION *ext;
            ext = X509_get_ext(cert, i);
            extstr = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));
            if (!strcmp(extstr, "subjectAltName"))
            {
                int j;
                const unsigned char *data;
                STACK_OF(CONF_VALUE) *val;
                CONF_VALUE *nval;
                const X509V3_EXT_METHOD *meth;
                if (!(meth = X509V3_EXT_get(ext)))
                    break;
                data = ext->value->data;
                val = meth->i2v(meth, meth->d2i(NULL, &data, ext->value->length), NULL);
                for (j = 0; j < sk_CONF_VALUE_num(val); j++)
                {
                    nval = sk_CONF_VALUE_value(val, j);
                    if (!strcmp(nval->name, "DNS"))
                    {
                        ok = 1;
                        break;
                    }
                }
            }
            if (ok)
                break;
#endif
        }
    }

    if (!ok && (subj = X509_get_subject_name(cert)) &&   //得到证书所有者的名字
        X509_NAME_get_text_by_NID(subj, NID_commonName, data, 256) > 0)
    {
        data[255] = 0;
        LOGPRINT(DEBUG, "the certificate subject name is : %s\n", data);

        strcpy(ca->commonName, data);
        if(!ca->cinema.empty())
            if (strcasecmp(data, ca->cinema.c_str()) != 0)
                goto err_occured;
    }

    evpPkey = X509_get_pubkey(cert);
    RsaSt = EVP_PKEY_get1_RSA(evpPkey);
    if(RsaSt->n != NULL)
    {
        BN_bn2bin(RsaSt->n, RsaN);
        int len = BN_num_bytes(RsaSt->n);
        unsigned char *pcPtr = RsaN;
        while(pcPtr < (RsaN + len))
        {
            sprintf((ca->caRsaN)+strlen(ca->caRsaN), "%02x", *pcPtr);
            pcPtr++;
        }
        LOGWRITEHEX(DEBUG, RsaN, len);
        LOGPRINT(DEBUG, "[%s]\n", ca->caRsaN);
    }
    if(RsaSt->e != NULL)
    {
        BN_bn2bin(RsaSt->e, RsaE);
        int len = BN_num_bytes(RsaSt->e);
        LOGWRITEHEX(DEBUG, RsaE, len);
    }

    X509_free(cert);
    return SSL_get_verify_result(ca->ssl);

err_occured:
    if (cert)
        X509_free(cert);
    return X509_V_ERR_APPLICATION_VERIFICATION;
}

#if defined(WIN32)
    #define MUTEX_TYPE HANDLE
    #define MUTEX_SETUP(x) (x) = CreateMutex(NULL, FALSE, NULL)
    #define MUTEX_CLEANUP(x) CloseHandle(x)
    #define MUTEX_LOCK(x) WaitForSingleObject((x), INFINITE)
    #define MUTEX_UNLOCK(x) ReleaseMutex(x)
    #define THREAD_ID GetCurrentThreadId()
#elif defined(_POSIX_THREADS)
/* _POSIX_THREADS is normally defined in unistd.h if pthreads are available on your platform. */
    #define MUTEX_TYPE pthread_mutex_t
    #define MUTEX_SETUP(x) pthread_mutex_init(&(x), NULL)
    #define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
    #define MUTEX_LOCK(x) pthread_mutex_lock(&(x))
    #define MUTEX_UNLOCK(x) pthread_mutex_unlock(&(x))
    #define THREAD_ID pthread_self()
#else
    #error You must define mutex operations appropriate for your platform!
#endif
/* This array will store all of the mutexes available to OpenSSL. */
static MUTEX_TYPE *mutex_buf = NULL;
static void locking_function(int mode, int n, const char * file, int line)
{
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(mutex_buf[n]);
    else
        MUTEX_UNLOCK(mutex_buf[n]);
}

static unsigned long id_function(void)
{
    return ((unsigned long)THREAD_ID);
}

struct CRYPTO_dynlock_value
{
    MUTEX_TYPE mutex;
};

static struct CRYPTO_dynlock_value *dyn_create_function(const char *file,int line)
{
    struct CRYPTO_dynlock_value *value;
    value = (struct CRYPTO_dynlock_value *)malloc(sizeof(struct  CRYPTO_dynlock_value));
    if (!value)
        return NULL;
    MUTEX_SETUP(value->mutex);
    return value;
}

static void dyn_lock_function(int mode, struct CRYPTO_dynlock_value *l,const char *file, int line)
{
    if (mode & CRYPTO_LOCK)
        MUTEX_LOCK(l->mutex);
    else
        MUTEX_UNLOCK(l->mutex);
}

static void dyn_destroy_function(struct CRYPTO_dynlock_value *l, const char *file, int line)
{
    MUTEX_CLEANUP(l->mutex);
    free(l);
}

int THREAD_setup(void)
{
    int i;
    mutex_buf = (MUTEX_TYPE *)malloc(CRYPTO_num_locks() *sizeof(MUTEX_TYPE));
    if (!mutex_buf)
        return 0;
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_SETUP(mutex_buf[i]);
    CRYPTO_set_id_callback(id_function);
    CRYPTO_set_locking_callback(locking_function);
    /* The following three CRYPTO_... functions are the OpenSSL functions
    for registering the callbacks we implemented above */
    CRYPTO_set_dynlock_create_callback(dyn_create_function);
    CRYPTO_set_dynlock_lock_callback(dyn_lock_function);
    CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function);
    return 1;
}

int THREAD_cleanup(void)
{
    int i;
    if (!mutex_buf)
        return 0;
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
        MUTEX_CLEANUP(mutex_buf[i]);
    free(mutex_buf);
    mutex_buf = NULL;
    return 1;
}

int seed_prng(int bytes)
{
    if (!RAND_load_file("/dev/random", bytes))
        return 0;
    return 1;
}

/*
if no /dev/random
int seed_prng(int bytes)
{
    int error;
    char *buf;
    prngctx_t ctx;
    egads_init(&ctx, NULL, NULL, &error);
    if (error)
        return 0;
    buf = (char *)malloc(bytes);
    egads_entropy(&ctx, buf, bytes, &error);
    if (!error)
        RAND_seed(buf, bytes);
    free(buf);
    egads_destroy(&ctx);
    return (!error);
}
*/

DH *tmp_dh_callback(SSL *ssl, int is_export, int keylength)
{
    DH *ret;

    if (!dh512 || !dh1024)
        init_dhparams();

    switch (keylength)
    {
        case 512:
            ret = dh512;
            break;
        case 1024:
        default: /* generating DH params is too costly to do on the fly */
            ret = dh1024;
            break;
    }
    return ret;
}

void init_dhparams(void)
{
    BIO *bio;

    bio = BIO_new_file("dh512.pem", "r");
    if (!bio)
        int_error("Error opening file dh512.pem");
    dh512 = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    if (!dh512)
        int_error("Error reading DH parameters from dh512.pem");
    BIO_free(bio);

    bio = BIO_new_file("dh1024.pem", "r");
    if (!bio)
        int_error("Error opening file dh1024.pem");
    dh1024 = PEM_read_bio_DHparams(bio, NULL, NULL, NULL);
    if (!dh1024)
        int_error("Error reading DH parameters from dh1024.pem");
    BIO_free(bio);
}

void MemDump(unsigned char *pucaAddr, int lLength)
{
    using namespace std;
    int  i,j,n;
    int  iPage = 20;
    char caTmp[100];
    char caBuf[1650] = {0};
    unsigned char *pcPtr;

    pcPtr=pucaAddr;
    while ( pcPtr < (pucaAddr + lLength))
    {
        for (j=0;j <= (lLength-1)/16 ; j++)
        {
            if (j == (j/iPage)*iPage)
            {
                strcpy(caTmp,"\nDisplacement ");
                strcat(caTmp,"-0--1--2--3--4--5--6--7--8--9--A--B--C--D--E--F");
                strcat(caTmp,"  --ASCII  Value--\n");
                std::cout << caTmp;
            }

            sprintf(caTmp, "%05d(%05x) ", j*16, j*16);
            for (i=0; (i<16)&&(pcPtr<(pucaAddr+lLength)); i++)
            {
                sprintf(caTmp + strlen(caTmp),"%02x ", *pcPtr);
                pcPtr++;
            }
            for (n=0; n<16-i; n++)
            {
                strcat(caTmp, "   ");
            }
            strcat(caTmp, " ");
            pcPtr = pcPtr - i;

            for (n=0; n<i; n++)
            {
                if( ((*pcPtr)<=31) && ((*pcPtr)>=0) )
                {
                    strcat(caTmp, ".");
                }
                else
                {
                    sprintf(caTmp + strlen(caTmp),"%c",*pcPtr);
                }
                pcPtr++;
            }

            strcat(caBuf,caTmp);
            strcat(caBuf,"\n");
            if (j == (j/iPage)*iPage)
            {
                std::cout << caBuf;
                caBuf[0]='\0';
            }
        } /* end of for */
    } /* end of while */

    std::cout << caBuf << endl;
} /* end of MemDump   */

