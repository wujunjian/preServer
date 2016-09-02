#pragma once
#include <string>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <sys/epoll.h>
#include <list>
#include <queue>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <algorithm>
#include <fcntl.h>
#include <semaphore.h>

using namespace std;
class CINEMACONNECT;
class PAYLOADCONNECT;

#pragma pack(1)
typedef struct __PACKHEAD__
{
    unsigned short sync_tag;
    unsigned char version;
    unsigned short packet_length;
    unsigned char payload_id;
}PACKHEAD;

class AuthBody
{
public:
    char username[33];
    char password[33];
    char software_MD5[33];
    unsigned int rawdata_count;
};
#pragma pack()

#define TOCHKCOUNT 100
#define VALIDTIME 10
#define CINEMAVALIDTIME 310
#define MAX_EVENTS 10
#define CIPHER_LIST "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH"
#define CAFILE "cacert.pem"
#define CADIR NULL
#define CERTFILE "servercert.pem"
#define PRIVATEKEY "serverkey.pem"
#define OPENSSL_THREAD_DEFINES
#define SSLCHK(SSL,RET) switch(SSL_get_error(SSL, RET))\
            {case SSL_ERROR_NONE:\
                LOGPRINT(DEBUG, "SSL_ERROR_NONE\n");\
                RET = 1;\
                break;\
            case SSL_ERROR_WANT_WRITE:\
                LOGPRINT(DEBUG, "SSL_ERROR_WANT_WRITE\n");\
                RET = 1;\
                break;\
            case SSL_ERROR_WANT_READ:\
                LOGPRINT(DEBUG, "SSL_ERROR_WANT_READ\n");\
                RET = 1;\
                break;\
            case SSL_ERROR_WANT_CONNECT:\
            case SSL_ERROR_WANT_ACCEPT:\
                LOGPRINT(DEBUG, "WANT_CONNECT/ACCEPT\n");\
                RET = 1;\
                break;\
            case SSL_ERROR_ZERO_RETURN:\
                LOGPRINT(DEBUG, "SSL_ERROR_ZERO_RETURN\n");\
                RET = -1;\
                break;\
            case SSL_ERROR_SYSCALL:\
                LOGPRINT(DEBUG, "SSL_ERROR_SYSCALL\n");\
                RET = -1;\
                break;\
            case SSL_ERROR_SSL:\
                LOGPRINT(DEBUG, "SSL_ERROR_SSL\n");\
                RET = -1;\
                break;\
            case SSL_ERROR_WANT_X509_LOOKUP:\
                LOGPRINT(DEBUG, "SSL_ERROR_WANT_X509_LOOKUP\n");\
                break;\
            default:\
                LOGPRINT(DEBUG, "SSL_ERROR_DEFAULT\n");\
                RET = -1;\
                break;}

class preServer
{
public:
    preServer();
    ~preServer();
    static preServer *instance();
    SSL_CTX *setup_server_ctx(void);
    void* do_server_loop(void *arg);
    void* do_unix_loop(void *arg);
    void* server_process(void *arg);
    void* unix_process(void *arg);
    static void *Auththread(void* arg);
    static void *AuthClear(void* arg);
    void *TaskRecord(void* arg);
    int Init();
    void recv();
    void send();
    void unixrecv();
    void unixsend();

    int Getepollfd(){return epollfd;};
    int Getepollunfd(){return epollunfd;};
    void DeleteCinema(string &cc);
    void DeletePayload(unsigned char pl);
    void GetSSLLOCK(){pthread_rwlock_wrlock(&CinemaMapLock);};
    void UNSSLLOCK(){pthread_rwlock_unlock(&CinemaMapLock);};

private:
    SSL_CTX *ctx;
    struct epoll_event ev, events[MAX_EVENTS];
    struct epoll_event unev, unevents[MAX_EVENTS];
    int listen_sock, conn_sock, epollfd;
    int listen_unixsock, conn_unixsock, epollunfd;
    map<string, CINEMACONNECT*>cinema;          //认证成功影院
    map<unsigned char, PAYLOADCONNECT*>payload;

    queue<char*> SockRecv;
    queue<char*> UnixSockRecv;
    queue<char*> qTaskRecord;

    pthread_mutex_t QueueSockRecv;
    pthread_mutex_t QueueUnixSockRecv;
    pthread_mutex_t QueueqTaskRecord;

    list<CINEMACONNECT*> qCinemaConn;           //连接队列
    queue<CINEMACONNECT*> qCinemaDel;           //延时销毁队列
    static preServer *l_Instance;

    //pthread_mutex_t CinemaMapLock;
    pthread_rwlock_t CinemaMapLock;
    //pthread_mutex_t PayLoadMaplock;
    pthread_rwlock_t PayLoadMaplock;

    //sem_t m_TcpRecvSem;
    //sem_t m_UnixRecvSem;
};

class CINEMACONNECT
{
public:
    CINEMACONNECT(int conn):clientport(0),fd(conn),ssl(NULL),data(NULL),datalen(0), authFlag(0), packFlag(0)
        {::time(&ltime); memset(headbuf, 0x00, sizeof(headbuf));memset(commonName, 0x00, sizeof(commonName));
        memset(caRsaN, 0x00, sizeof(caRsaN));memset(caRsaE, 0x00, sizeof(caRsaE));};
    ~CINEMACONNECT();

    void remove();               //从map中清除
    bool chkRsaStat();          //true 正常, false 异常

    int authFlag;               /// 是否可以接收认证包
    int packFlag;               /// 包状态
    int fd;
    time_t ltime;
    SSL* ssl;
    string cinema;
    string clientip;
    unsigned short clientport;
    char* data;             //正在接收数据地址
    unsigned int datalen;   //已接收数据长度
    unsigned char headbuf[6];

    string m_PubKey;
    string m_BusinessStatus;
    char commonName[256];      //证书所有者的名字  与 后面取出cinema做对比, 不符则认证失败
    char caRsaN[256+1];
    char caRsaE[256+1];
};

class PAYLOADCONNECT
{
public:
    PAYLOADCONNECT():fd(0),data(NULL),datalen(0)
        {memset(headbuf, 0x00, sizeof(headbuf));memset(payload, 0x00, sizeof(payload));};
    ~PAYLOADCONNECT();
    int fd;
    char* data;                 //正在接收数据地址
    unsigned char payload[0x28];//与此连接关联的payload
    unsigned int datalen;       //已接收数据长度
    char headbuf[9+6];          //
};
