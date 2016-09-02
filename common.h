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
#include <iostream>

#ifndef WIN32
    #include <pthread.h>
    #define THREAD_CC
    #define THREAD_TYPE pthread_t
    #define THREAD_CREATE(tid, entry, arg) \
            {pthread_attr_t attr;\
            pthread_attr_init(&attr);\
            pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);\
            pthread_create(&(tid), &attr, (entry), (arg));\
            pthread_attr_destroy(&attr);}

#else
    #include <windows.h>
    #define THREAD_CC _ _cdecl
    #define THREAD_TYPE DWORD
    #define THREAD_CREATE(tid, entry, arg) do { _beginthread((entry), 0,\
                        (arg)); (tid) = GetCurrentThreadId(); \
                        } while (0)
#endif

#define PORT "*:6001"
#define int_error(msg) handle_error(__FILE__, __LINE__, msg)

void handle_error(const char *file, int lineno, const char *msg);
void init_OpenSSL(void);
int verify_callback(int ok, X509_STORE_CTX *store);
class CINEMACONNECT;
long post_connection_check(CINEMACONNECT *arg);
int THREAD_setup(void);
int THREAD_cleanup(void);
int seed_prng(int bytes);
void init_dhparams(void);
DH *tmp_dh_callback(SSL *ssl, int is_export, int keylength);
void MemDump(unsigned char *pucaAddr, int lLength);

