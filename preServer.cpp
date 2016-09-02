#include "preServer.h"
#include "common.h"
#include "DbHelper.h"
#include "RSAHelper.h"
#include "Utils.h"
#include "CRC.h"
#include "BaseLog.h"
#include "Config.h"
#include "BroadCast.h"

preServer *preServer::l_Instance = NULL;
CINEMACONNECT::~CINEMACONNECT()
{
    if(data != NULL)
    {
        free(data);
        data = NULL;
    }
    if(ssl != NULL)
    {
        if((SSL_get_shutdown(ssl) & SSL_RECEIVED_SHUTDOWN) ? 1 : 0)
            if(!SSL_shutdown(ssl))
                SSL_shutdown(ssl);
        else
            SSL_clear(ssl);
        SSL_free(ssl);
    }
    if(fd != 0)
    {
        close(fd);
    }
    //ERR_remove_state(0);
}

bool CINEMACONNECT::chkRsaStat()
{
    char sql[512] = {0};
    strcpy(sql, "select t0.tls_sec_key from Y_T_UK_CER_INFO_DETAIL t0 "
                "right join Y_T_UK_CER_INFO t1  on t0.cer_info_id = t1.id "
                "where t1.cinema_code = '");
    strcat(sql, commonName);
    strcat(sql, "' and t0.status = '0' and rownum = 1 ");
    try
    {
        DbHelper helper;
        int result = helper.DoQuery(string(sql));
        if (result == 0)
        {
            // 查找到该影院
            if (helper.GetRs() != NULL && helper.GetRs()->next())
            {
                LOGPRINT(DEBUG, "find the cinema [%s]\n", commonName);
                string tls_sec_key = helper.GetRs()->getString(1);
                LOGPRINT(DEBUG, "tls_sec_key [%s]\n", tls_sec_key.c_str());
                LOGPRINT(DEBUG, "caRsaN [%s]\n", caRsaN);
                //if(tls_sec_key == caRsaN)
                if(!tls_sec_key.compare(caRsaN))
                    return true;
                else
                    return false;
            }
            else
            {
                LOGPRINT(DEBUG, "Do not found the cinema [%s]\n", commonName);
                return false;
            }
        }
        else
        {
            LOGPRINT(DEBUG, "DoQuery return [%d]\n", result);
            return false;
        }
    }
    catch (SQLException& ex)
    {
        LOGPRINT(DEBUG, "SQLException Error Number : %d,Error Message : %s \n", ex.getErrorCode(), 
                         ex.getMessage().c_str());

        return false;
    }
    catch (exception& e)
    {
        LOGPRINT(DEBUG, "catch exception, [%s]\n", e.what());
        return false;
    }

    return true;
}

void CINEMACONNECT::remove()
{
    if(fd != 0)
    {
        epoll_ctl(preServer::instance()->Getepollfd(), EPOLL_CTL_DEL, fd, NULL);
    }
    preServer::instance()->DeleteCinema(cinema);
}

PAYLOADCONNECT::~PAYLOADCONNECT()
{
    if(data != NULL)
    {
        free(data);
        data = NULL;
    }
    if(fd != 0)
    {
        close(fd);
        epoll_ctl(preServer::instance()->Getepollunfd(), EPOLL_CTL_DEL, fd, NULL);
    }
    for(unsigned char pl = 0x01; pl <= 0x28; pl++)
    {
        if(payload[pl] != 0x00)
            preServer::instance()->DeletePayload(pl);
    }
}

void preServer::DeleteCinema(string &cc)
{
    pthread_rwlock_wrlock(&CinemaMapLock);
    map<string, CINEMACONNECT*>::iterator cinema_iter = cinema.find(cc);
    if (cinema_iter != cinema.end())
    {
        cinema.erase(cinema_iter);
    }
    pthread_rwlock_unlock(&CinemaMapLock);
}

void preServer::DeletePayload(unsigned char pl)
{
    pthread_rwlock_wrlock(&PayLoadMaplock);
    map<unsigned char, PAYLOADCONNECT*>::iterator iter = payload.find(pl);
    if (iter != payload.end())
    {
        payload.erase(iter);
    }
    pthread_rwlock_unlock(&PayLoadMaplock);
}

preServer::preServer()
{
    //pthread_mutex_init(&CinemaMapLock, NULL);
    //pthread_mutex_init(&PayLoadMaplock, NULL);

    pthread_rwlock_init(&CinemaMapLock, NULL);
    pthread_rwlock_init(&PayLoadMaplock, NULL);
    
    pthread_mutex_init(&QueueSockRecv, NULL);
    pthread_mutex_init(&QueueUnixSockRecv, NULL);
    pthread_mutex_init(&QueueqTaskRecord, NULL);

    //sem_init(&m_TcpRecvSem, 0, 0);
    //sem_init(&m_UnixRecvSem, 0, 0);
    ctx = NULL;
    listen_sock = 0;
    conn_sock = 0;
    epollfd = 0;
    listen_unixsock = 0;
    conn_unixsock = 0;
    epollunfd = 0;
}

preServer::~preServer()
{
    //pthread_mutex_destroy(&CinemaMapLock);
    //pthread_mutex_destroy(&PayLoadMaplock);
    pthread_rwlock_destroy(&CinemaMapLock);
    pthread_rwlock_destroy(&PayLoadMaplock);
    
    pthread_mutex_destroy(&QueueSockRecv);
    pthread_mutex_destroy(&QueueUnixSockRecv);
    pthread_mutex_destroy(&QueueqTaskRecord);
    SSL_CTX_free(ctx);
}

SSL_CTX *preServer::setup_server_ctx(void)
{
    SSL_CTX *ctx;

    //ctx = SSL_CTX_new(TLSv1_server_method());
    ctx = SSL_CTX_new(TLSv1_method());
    if (SSL_CTX_load_verify_locations(ctx, Config::instance()->CaCertPath.c_str(), CADIR) != 1)// CA 证书
        int_error("Error loading CA file and/or directory");
    if (SSL_CTX_set_default_verify_paths(ctx) != 1)                  // 默认CA证书
        int_error("Error loading default CA file and/or directory");
    if (SSL_CTX_use_certificate_chain_file(ctx, Config::instance()->ServerCertPath.c_str()) != 1)// 服务器证书文件
        int_error("Error loading certificate from file");
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)"Server15Pwd4803");   //私钥密码
    if (SSL_CTX_use_PrivateKey_file(ctx, Config::instance()->PriKeyPath.c_str(), SSL_FILETYPE_PEM) != 1)//私钥文件
        int_error("Error loading private key from file");
    if (!SSL_CTX_check_private_key(ctx))
        int_error("Private key does not match the certificate public key\n");

    /* returns the new verification status use verify_callback  */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|
                            SSL_VERIFY_FAIL_IF_NO_PEER_CERT|
                            SSL_VERIFY_CLIENT_ONCE, verify_callback);
    SSL_CTX_set_verify_depth(ctx, 2);
    SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE);
    //SSL_CTX_set_tmp_dh_callback(ctx, tmp_dh_callback);
    if (SSL_CTX_set_cipher_list(ctx, CIPHER_LIST) != 1)     //支持加密算法列表
        int_error("Error setting cipher list (no valid ciphers)");

    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_quiet_shutdown(ctx, 1);

    return ctx;
}


void* preServer::do_server_loop(void* arg)
{
    CINEMACONNECT *ca = static_cast<CINEMACONNECT *>(arg);
    int err, nbuffered;

    LOGPRINT(DEBUG, ":::: recv data ::::\n");
    // 读取报文头
    //nbuffered = SSL_pending(ca->ssl);
    //if(nbuffered < sizeof(ca->headbuf))
    //{
    //    LOGPRINT(DEBUG, "read head:not enough buffered[%d] to read from\n", nbuffered);
    //}

    for (;ca->datalen < sizeof(ca->headbuf); ca->datalen += err)
    {
        err = SSL_read(ca->ssl, ca->headbuf + ca->datalen, sizeof(ca->headbuf) - ca->datalen);
        if(err < 0)
        {
            //SSLCHK(ca->ssl,err);
            break;
        }
        else if(err == 0)
        {
            LOGPRINT(DEBUG, "the SSL connect is be Closed\n");
            epoll_ctl(epollfd, EPOLL_CTL_DEL, ca->fd, NULL);
            break;
        }
    }
    if(ca->datalen < 6)   //数据未收完整
        return NULL;

    if(ca->headbuf[0] != 0xAA || ca->headbuf[1] != 0x55 || ca->headbuf[2] != 0x01)
    {
        //message is illegal
        LOGPRINT(DEBUG, "the cinema [%s]'s message is illegal.", ca->commonName);
        LOGWRITEHEX(DEBUG, ca->headbuf, sizeof(ca->headbuf));
        epoll_ctl(epollfd, EPOLL_CTL_DEL, ca->fd, NULL);
        return NULL;
    }
    PACKHEAD *head = (PACKHEAD *)(ca->headbuf);
    //nbuffered = SSL_pending(ca->ssl);
    //if(nbuffered < (head->packet_length - 6))
    //{
    //    LOGPRINT(DEBUG, "read body:not enough buffered[%d] to read from\n", nbuffered);
    //}

    if(ca->data == NULL)
    {
        ca->data = (char*)malloc(head->packet_length+9);   //发送之后free
    }
    memcpy(ca->data+9, ca->headbuf, 6);
    // 读取报文体
    for (; ca->datalen < head->packet_length; ca->datalen += err)
    {
        if(ca->authFlag == 1 || ca->data == NULL)   // 正在认证, 或者认证完成,该空间已经释放.
        {
            ca->packFlag = 1;                       // 该包丢弃
            char Tmp[(head->packet_length)+9];
            err = SSL_read(ca->ssl, Tmp + ca->datalen + 9, head->packet_length - ca->datalen);
        }
        else
        {
            err = SSL_read(ca->ssl, ca->data + ca->datalen + 9, head->packet_length - ca->datalen);
        }

        if (err < 0)
        {
            //SSLCHK(ca->ssl,err);
            break;
        }
        else if(err == 0)
        {
            LOGPRINT(DEBUG, "the SSL connect is be Closed\n");
            epoll_ctl(epollfd, EPOLL_CTL_DEL, ca->fd, NULL);
            break;
        }
    }

    if(ca->datalen < head->packet_length)   //数据未完整接收
    {
        LOGPRINT(DEBUG, "already recv length %u", ca->datalen);
        LOGWRITEHEX(DEBUG, ca->data+9, ca->datalen);
        LOGPRINT(DEBUG, "continue to receive\n");
        return NULL;
    }

    ca->datalen = 0;
    if(ca->authFlag == 1 || ca->data == NULL || ca->packFlag == 1)
    {
        ca->packFlag = 0;
        return NULL;
    }

    LOGWRITEHEX(DEBUG, ca->data+9, head->packet_length);
    if (head->payload_id == 0x01)
    {
        if(ca->cinema.empty())  //认证线程
        {
            ca->authFlag = 1;
            THREAD_TYPE tid;
            LOGPRINT(DEBUG, "new thread to authenticate the cinema\n");
            THREAD_CREATE(tid, Auththread, arg);
        }
        else                    //已经认证报文,丢弃
        {
            LOGPRINT(DEBUG, "the cinema already authenticated\n");
            free(ca->data);
            ca->data = NULL;
            return NULL;
        }
    }
    else
    {
        if(ca->cinema.empty())  //未认证报文,丢弃
        {
            LOGPRINT(DEBUG, "the cinema is not already authenticated\n");
            free(ca->data);
            ca->data = NULL;
            return NULL;
        }
        else                            //加入消息队列,等待转发
        {
            time(&(ca->ltime));
            memcpy(ca->data, ca->cinema.c_str(), 9);
            if(head->payload_id == 0x03)
            {
                LOGPRINT(DEBUG, "receive cinema[%s]'s %02x\n", ca->cinema.c_str(), head->payload_id);
/*
                if(ca->chkRsaStat() == false)
                {
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, ca->fd, NULL);
                }
                else
                {
*/
                    string Msg = ca->cinema + ":" + Config::instance()->ServerIP;
                    BroadCast::SendBroadCast(Msg.c_str()); /// 更新影院时间
/*
                }
*/
                free(ca->data);
            }
            else
            {
                /* 排查map中找不到影院连接现象 
                pthread_rwlock_rdlock(&CinemaMapLock);
                    map<string, CINEMACONNECT*>::iterator iter = cinema.find(ca->cinema);
                    if (iter == cinema.end())
                    {
                        LOGPRINT(DEBUG, "the cinema [%s] is not in the map", ca->cinema.c_str());
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, ca->fd, NULL);
                        free(ca->data);
                        ca->data = NULL;
                        pthread_rwlock_unlock(&CinemaMapLock);
                        return NULL;
                    }
                pthread_rwlock_unlock(&CinemaMapLock);
                */

                LOGPRINT(DEBUG, "push the message to queue for send\n");
                pthread_mutex_lock(&QueueSockRecv);
                SockRecv.push(ca->data);   // 插入前可以检查队列深度.过大则丢弃该报文.
                pthread_mutex_unlock(&QueueSockRecv);
                //sem_post(&m_TcpRecvSem);
            }
            ca->data = NULL;
        }
    }

    LOGPRINT(DEBUG, "message recv success\n");
    return NULL;
}

void *preServer::AuthClear(void* arg)
{
    sleep(2);
    CINEMACONNECT* sp;
    
    while(1)
    {
        if(preServer::instance()->qCinemaDel.empty())
        {
            break;
        }
        sp = preServer::instance()->qCinemaDel.front();
        delete sp;
        preServer::instance()->qCinemaDel.pop();
    }
    return NULL;
}

void *preServer::Auththread(void* arg)
{

    LOGINIT("CINEMALOGIN");
#ifndef WIN32
    pthread_detach(pthread_self());
#endif
    unsigned char  return_value = 5; /// 0=成功 1=无此用户 2=证书不匹配 3=版本不匹配 4=服务暂停 5=未知错误
    CINEMACONNECT *ca = static_cast<CINEMACONNECT *>(arg);
    PACKHEAD *head = (PACKHEAD *)(ca->data+9);

    //验证
    LOGPRINT(DEBUG, "the Auththread ...[%ld]\n", pthread_self());

    AuthBody *body = reinterpret_cast<AuthBody *>(ca->data + 9 + sizeof(PACKHEAD));

    char sql[512] = {0};
    strcpy(sql, "SELECT ci.CODE,ci.BUSINESS_STATUS,ci.STATUS,ucid.SEC_KEY,uci.CINEMA_PWD"
    " FROM Y_T_CINEMA_INFO ci"
    " LEFT JOIN Y_T_UK_CER_INFO uci ON uci.CINEMA_CODE=ci.CODE AND uci.CER_TYPE=1"
    " LEFT JOIN Y_T_UK_CER_INFO_DETAIL ucid ON ucid.CER_INFO_ID=uci.ID AND ucid.STATUS=0"
    " WHERE ci.CODE='");
    strcat(sql, body->username);
    strcat(sql, "'");

    LOGPRINT(DEBUG, "username = %s, commonName = %s \n", body->username, ca->commonName);
    LOGPRINT(DEBUG, "client = %s, port = %hu \n", ca->clientip.c_str(), ca->clientport);
    if(0 == strncmp(body->username, "0", 1)) //以0开头的全部为压力测试数据,不校验
    {
        return_value = 0;
        goto pressure_test; 
    }
    if(strcmp(body->username, ca->commonName))
    {
        return_value = 2;
        goto logon_response;
    }
    if(ca->chkRsaStat() == false)
    {
        //return_value = 1;
        //goto logon_response;
    }

    LOGPRINT(DEBUG, "sql = [%s]\n", sql);
    /* 认证 */
    try
    {
        DbHelper helper;
        int result = helper.DoQuery(string(sql));
        if (result == 0)
        {
            // 查找到该影院
            if (helper.GetRs() != NULL && helper.GetRs()->next())
            {
                LOGPRINT(DEBUG, "find the cinema [%s]\n", body->username);
                return_value = 0;

                string businessstatus = helper.GetRs()->getString(2);
                string hexkey = helper.GetRs()->getString(4);
                string pwd = helper.GetRs()->getString(5);
                string pwdmd5 = RSAHelper::MD5ToStr(pwd.c_str());

                char dbpwd[16];
                char inpwd[16];
                Utils::HexStrToBin(pwdmd5.c_str(), 32, dbpwd);
                Utils::HexStrToBin(body->password, 32, inpwd);

                // 营业状态（0待审核，1测试，2营业，3停业，4注销）
                if (hexkey.length() == 263
                    && businessstatus != "0" && businessstatus != "4"
                    && memcmp(dbpwd, inpwd, 16) == 0)
                {
                    ca->m_PubKey = hexkey.substr(0, 256);
                    ca->m_BusinessStatus = businessstatus;
                }
                else
                {
                    LOGPRINT(DEBUG, "Public key Or businessstatus Or password is wrong[%s].\n"
                                    "password MD5 [%s], peer password MD5 [%s]\n", body->username, pwdmd5.c_str(), body->password);

                    LOGPRINT(DEBUG, "hexkey.length()=%d, businessstatus=%s", hexkey.length(), businessstatus.c_str());
                    return_value = 4;
                }
            }
            else
            {
                LOGPRINT(DEBUG, "Do not found the cinema [%s]\n", body->username);
                return_value = 1;
            }
        }
        else
        {
            LOGPRINT(DEBUG, "DoQuery return [%d]\n", result);
            return_value = 5;
        }
    }
    catch (SQLException& ex)
    {
        LOGPRINT(DEBUG, "SQLException Error Number : %d,Error Message : %s \n", ex.getErrorCode(), 
                         ex.getMessage().c_str());
                         
        free(ca->data);
        ca->data = NULL;
        ca->authFlag = 0;
        return NULL;
    }
    catch (exception& e)
    {
        LOGPRINT(DEBUG, "catch exception\n");
        free(ca->data);
        ca->data = NULL;
        ca->authFlag = 0;
        return NULL;
    }

pressure_test:
    if(return_value == 0) //登陆成功
    {
        preServer::instance()->GetSSLLOCK();
        //如果该cinema已存在,则需删除上一个连接
        if(!ca->cinema.empty())
        {
            //一个连接同时过来俩个验证包. 会导致第2个验证包错误的删除连接.
            preServer::instance()->UNSSLLOCK();
            LOGPRINT(DEBUG, "cinemaCode[%s] have already login success\n", ca->cinema.c_str());
            goto logon_response;
        }

        ca->cinema = body->username;
        LOGPRINT(DEBUG, "cinemaCode[%s] login success\n", ca->cinema.c_str());

        map<string, CINEMACONNECT*>::iterator iter = preServer::instance()->cinema.find(ca->cinema);
        if (iter != preServer::instance()->cinema.end())
        {
            LOGPRINT(DEBUG, "the cinema[%s] is already existed\n", ca->cinema.c_str());
            epoll_ctl(preServer::instance()->Getepollfd(), EPOLL_CTL_DEL, iter->second->fd, NULL);
            iter->second->cinema.clear();         //不能删除
            preServer::instance()->cinema.erase(iter);
        }
        preServer::instance()->cinema.insert(map<string, CINEMACONNECT*>::value_type(ca->cinema, ca));
        preServer::instance()->UNSSLLOCK();

        string Msg = ca->cinema + ":" + Config::instance()->ServerIP;
        BroadCast::SendBroadCast(Msg.c_str());
        Msg = ca->cinema + "PUBKEY:" + ca->m_PubKey;
        BroadCast::SendBroadCast(Msg.c_str());
    }
logon_response:
    char sendbuf[42];
    sprintf(sendbuf, "%c%c%c%c%c%c%c%33s", 0xAA,0x55,0x01,0x2A,0x00,0x02,return_value,
        "89ba1638b67ffaa6d96b4f07b6a3e1a6");
    unsigned short crc = CalcCrc16((unsigned char *)sendbuf, 40);
    //sprintf(sendbuf+40, "%hu", crc);
    *((unsigned short *)(sendbuf + 40)) = crc;

    SSL_write(ca->ssl, sendbuf, 42);
    LOGWRITEHEX(DEBUG, sendbuf, 42);

    free(ca->data);
    ca->data = NULL;
    ca->authFlag = 0;
    return NULL;
}

void* preServer::server_process(void *arg)
{
    CINEMACONNECT *ca = static_cast<CINEMACONNECT *>(arg);
    LOGPRINT(DEBUG, "client %s : %hu\n", ca->clientip.c_str(), ca->clientport);

    do_server_loop(arg);
    return NULL;
}

void* preServer::unix_process(void *arg)
{
    do_unix_loop(arg);
    return NULL;
}

void* preServer::do_unix_loop(void *arg)
{
    PAYLOADCONNECT *pc = static_cast<PAYLOADCONNECT *>(arg);
    int err;

    // 读取报文头
    LOGPRINT(DEBUG, "unix socket read from %d\n", pc->fd);
    for (; pc->datalen < sizeof(pc->headbuf); pc->datalen += err)
    {
        err = read(pc->fd, pc->headbuf + pc->datalen, sizeof(pc->headbuf) - pc->datalen);
        if (err < 0)
        {
            LOGPRINT(DEBUG, "read unix socket %d err, errno = %d \n", pc->fd, errno);
            break;
        }
        else if(err == 0)
        {
            LOGPRINT(DEBUG, "unix socket %d is be closed \n", pc->fd);
            epoll_ctl(epollunfd, EPOLL_CTL_DEL, pc->fd, NULL);
            delete pc;
            return NULL;
        }
    }
    if(pc->datalen < 9+6)   //数据未收完整
        return NULL;

    PACKHEAD *head = (PACKHEAD *)(pc->headbuf+9);
    if(pc->data == NULL)
        pc->data = (char*)malloc(head->packet_length + 9 + 32);   //发送之后free +32为报文尾部任务id
    memcpy(pc->data, pc->headbuf, sizeof(pc->headbuf));
    // 读取报文体
    for (; pc->datalen - 9 < (head->packet_length + 32); pc->datalen += err)
    {
        err = read(pc->fd, pc->data + pc->datalen, head->packet_length + 32 - pc->datalen + 9);
        if (err < 0)
        {
            LOGPRINT(DEBUG, "read unix socket %d err, errno = %d \n", pc->fd, errno);
            break;
        }
        else if(err == 0)
        {
            LOGPRINT(DEBUG, "unix socket %d is be closed \n", pc->fd);
            epoll_ctl(epollunfd, EPOLL_CTL_DEL, pc->fd, NULL);
            delete pc;
            return NULL;
        }
    }
    if(pc->datalen < head->packet_length + 32 + 9)   //数据未完整接收
        return NULL;
    LOGWRITEHEX(DEBUG, pc->data+9, pc->datalen - 9);

    pc->datalen = 0;
    if (0 == memcmp(pc->headbuf, "00000000", 8))  // payload注册
    {
        unsigned char capayload;
        for(char* p = pc->data + 9 + 6; *p != 0x00; p++)
        {
            capayload = p[0];
            pthread_rwlock_rdlock(&PayLoadMaplock);
            map<unsigned char, PAYLOADCONNECT*>::iterator iter = preServer::instance()->payload.find(capayload);
            pthread_rwlock_unlock(&PayLoadMaplock);
            if (iter != preServer::instance()->payload.end())
            {
                if(pc == iter->second)
                {
                    // payload重新注册(同一个连接)
                    if(p == pc->data+9+6)
                    {
                        // 清除map.
                        for(unsigned char pl = 0x01; pl <= 0x28; pl++)
                        {
                            if(pc->payload[pl] != 0x00)
                                preServer::instance()->DeletePayload(pl);
                        }
                        memset(pc->payload, 0x00, sizeof(pc->payload));
                    }
                }
                else
                    delete iter->second;
            }
            pthread_rwlock_wrlock(&PayLoadMaplock);
            pc->payload[capayload] = 0xff;
            preServer::instance()->payload.insert(map<unsigned char, PAYLOADCONNECT*>::value_type(capayload, pc));
            pthread_rwlock_unlock(&PayLoadMaplock);
        }

        free(pc->data);
    }
    else
    {
        pthread_mutex_lock(&QueueUnixSockRecv);
        UnixSockRecv.push(pc->data);   // 插入前可以检查队列深度.过大则丢弃该报文.
        pthread_mutex_unlock(&QueueUnixSockRecv);
        //sem_post(&m_UnixRecvSem);
    }
    pc->data = NULL;

    LOGPRINT(DEBUG, "unix message recv success\n");
    return NULL;
}


preServer *preServer::instance()
{
    if (l_Instance == NULL)
        l_Instance = new preServer();

    return l_Instance;
}

int preServer::Init()
{
    init_OpenSSL();
    seed_prng(1);

    const int on = 1;
    int nNetTimeout=1000;//1秒

    struct sockaddr_in serv;
#ifndef SOCK_NONBLOCK
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
#else
    listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
#endif
    if (listen_sock < 0)
        return -1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(listen_sock, SOL_SOCKET, SO_SNDTIMEO, &nNetTimeout, sizeof(int));
    setsockopt(listen_sock, SOL_SOCKET, SO_RCVTIMEO, &nNetTimeout, sizeof(int));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(Config::instance()->ListenPort);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listen_sock, (struct sockaddr*)&serv, sizeof(struct sockaddr)) < 0)
        int_error("bind listen_sock err");
    listen(listen_sock, 150);
    fcntl(listen_sock, F_SETFL, O_NONBLOCK);

    // unix socket
    //listen_unixsock = socket(AF_UNIX, SOCK_STREAM, 0);
#ifndef SOCK_NONBLOCK
    if(Config::instance()->Unixport == 0)
        listen_unixsock = socket(AF_UNIX, SOCK_STREAM, 0);
    else
        listen_unixsock = socket(AF_INET, SOCK_STREAM, 0);
#else
    if(Config::instance()->Unixport == 0)
        listen_unixsock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    else
        listen_unixsock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
#endif
    if (listen_unixsock < 0)
        return -1;
    
    setsockopt(listen_unixsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(listen_unixsock, SOL_SOCKET, SO_SNDTIMEO, &nNetTimeout, sizeof(int));
    setsockopt(listen_unixsock, SOL_SOCKET, SO_RCVTIMEO, &nNetTimeout, sizeof(int));

    if(Config::instance()->Unixport == 0)
    {
        struct sockaddr_un servu;
        servu.sun_family = AF_UNIX;
        strncpy(servu.sun_path, Config::instance()->UnixSockFile.c_str(), sizeof(servu.sun_path) - 1);
        if(bind(listen_unixsock, (struct sockaddr*)&serv, sizeof(struct sockaddr_un)) < 0)
            int_error("bind listen_unixsock err");

        unlink(Config::instance()->UnixSockFile.c_str());
    }
    else
    {
        serv.sin_family = AF_INET;
        serv.sin_port = htons(Config::instance()->Unixport);
        serv.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(listen_unixsock, (struct sockaddr*)&serv, sizeof(struct sockaddr)) < 0)
            int_error("bind listen_unixsock err");
    }

    listen(listen_unixsock, 150);
    fcntl(listen_unixsock, F_SETFL, O_NONBLOCK);

    epollfd = epoll_create(MAX_EVENTS);
    if (epollfd == -1)
    {
        int_error("epoll_create");
        exit(EXIT_FAILURE);
    }
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        int_error("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    //unix socket
    epollunfd = epoll_create(MAX_EVENTS);
    if (epollunfd == -1)
    {
        int_error("epoll_create");
        exit(EXIT_FAILURE);
    }
    unev.events = EPOLLIN;
    unev.data.ptr = NULL;
    if (epoll_ctl(epollunfd, EPOLL_CTL_ADD, listen_unixsock, &unev) == -1)
    {
        int_error("epoll_ctl: listen_unixsock");
        exit(EXIT_FAILURE);
    }

    ctx = setup_server_ctx();
}

void preServer::recv()
{
    int nfds, TotalCount = 0, InvalidCount = 0;
    struct sockaddr_in client;
    socklen_t clientsocklen;
    long lastChkTime;
    time(&lastChkTime);
    for (;;)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            if (errno == EINTR) /* Interrupted system call */
            {
                LOGPRINT(DEBUG, "Interrupted system call");
                continue;
            }

            int_error("epoll_wait");
            exit(EXIT_FAILURE);
        }
        LOGPRINT(DEBUG, "\n\n ********************** [%d]active ********************** \n", nfds);
        for (int n = 0; n < nfds; ++n)
        {
            if((((0==TotalCount%TOCHKCOUNT) && time(NULL)>(lastChkTime+VALIDTIME))
                || (time(NULL)>(lastChkTime+CINEMAVALIDTIME)))
                    && !qCinemaConn.empty())
            {
                time(&lastChkTime);
                LOGPRINT(DEBUG, "Total Accept Count : Total:%d,Active:%d,Invalid:%d", 
                                TotalCount, TotalCount-InvalidCount, InvalidCount);
                list<CINEMACONNECT*>::iterator iter = qCinemaConn.begin();
                while(iter != qCinemaConn.end())
                {
                    if((time(NULL) > ((*iter)->ltime + VALIDTIME) && (*iter)->cinema.empty())
                        || (time(NULL) > ((*iter)->ltime + CINEMAVALIDTIME))
                        || ((*iter)->fd == 0))
                    {
                        LOGPRINT(DEBUG, "the connect is lose efficacy, socketfd[%d],cinema[%s],lasttime[%d]",
                                (*iter)->fd, ((*iter)->cinema).c_str(), (*iter)->ltime);

                        (*iter)->remove();
                        qCinemaDel.push(*iter); 
                        list<CINEMACONNECT*>::iterator itertmp = qCinemaConn.erase(iter);
                        iter = itertmp;
                        InvalidCount++;
                    }
                    else
                    {
                        LOGPRINT(DEBUG, "the connect is active : socketfd[%d],cinema[%s],lasttime[%d]",
                                (*iter)->fd, ((*iter)->cinema).c_str(), (*iter)->ltime);
                        iter++;
                    }
                }
                
                if(!qCinemaDel.empty())
                {
                    LOGPRINT(DEBUG, "clean the dirty connect, count[%d]\n", qCinemaDel.size());
                    THREAD_TYPE tid;
                    /* new thread to delete */
                    THREAD_CREATE(tid, AuthClear, NULL);

                    //LOGPRINT(DEBUG, "clean the dirty connect complete[%d]\n", qCinemaDel.size());
                }
            }
            LOGPRINT(DEBUG, "NU.%02d : \n", n);
            if (events[n].data.ptr == NULL)
            {
                TotalCount++;
                conn_sock = accept(listen_sock, (struct sockaddr*)&client, &clientsocklen);
                if (conn_sock == -1)
                {
                    if ((errno==EAGAIN) || (errno==EINTR))
                    {
                        usleep(1000);
                        LOGPRINT(DEBUG, "TCP accept Try again \n");
                        continue;
                    }
                    else if(errno == EMFILE)
                    {
                        // Too many open files
                        usleep(1000);
                        LOGPRINT(DEBUG, "errno : EMFILE\n");
                        continue;
                    }
                    int_error("accept");
                }
                fcntl(conn_sock, F_SETFL, O_NONBLOCK);
                LOGPRINT(DEBUG, "client %s : %d connect socket fd : %d\n",
                         inet_ntoa(client.sin_addr), ntohs(client.sin_port), conn_sock);

                CINEMACONNECT *c = new CINEMACONNECT(conn_sock);
                c->clientip = inet_ntoa(client.sin_addr);
                c->clientport = ntohs(client.sin_port);
                ev.events = EPOLLIN;
                ev.data.ptr = c;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1)
                {
                    int_error("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
                qCinemaConn.push_back(c);
            }
            else if((static_cast<CINEMACONNECT *>(events[n].data.ptr))->ssl == NULL)
            {
                CINEMACONNECT * conn = static_cast<CINEMACONNECT *>(events[n].data.ptr);
                SSL *ssl = NULL;
                if (!(ssl = SSL_new(ctx)))
                    int_error("Error creating SSL context");
                SSL_set_accept_state(ssl);
                if(!SSL_set_fd(ssl, conn->fd))
                {
                    LOGPRINT(DEBUG, "Error SSL_set_fd [%d]", conn->fd);
                }
                LOGPRINT(DEBUG, "SSL_set_fd [%d]", conn->fd);

                int status, ncount = 0;
                do
                {
                    status = ::SSL_accept(ssl);
                    SSLCHK(ssl,status);
                    usleep(100000);
                }
                while (++ncount < 30 && status == 1 && !SSL_is_init_finished (ssl));
                //while (++ncount < 30 && status == 1);

                if(status == 1 && ncount < 30)
                {
                    conn->ssl = ssl;
                    LOGPRINT(DEBUG, "SSL_accept finished\n");
                    
                    long err;
                    if ((err = post_connection_check(conn)) != X509_V_OK)
                    {
                        LOGPRINT(DEBUG, "-Error: peer certificate: %s\n", X509_verify_cert_error_string(err));
                    }
                    ev.events = EPOLLIN;
                    ev.data.ptr = events[n].data.ptr;
                    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &ev) == -1)
                    {
                        int_error("epoll_ctl: conn->fd");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    LOGPRINT(DEBUG, "SSL_accept overtime or err\n");
                    SSL_free(ssl);
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
                    close(conn->fd);
                    conn->fd = 0;
                }
            }
            else
            {
                server_process(events[n].data.ptr);
            }
        }
    }
}

void preServer::send()
{
    char *sp;
    PACKHEAD* head;
    int sendlen, ret, isemRet;
    while(1)
    {
        if(SockRecv.empty())
        {
            usleep(100);
            continue;
        }
        //isemRet = sem_wait(&m_TcpRecvSem);
        //if (isemRet == -1 && errno == EINTR)
        //{
        //    continue;
        //}

        sp = SockRecv.front();
        sendlen = 0;
        ret = 0;

        head = (PACKHEAD*)(sp+9);

        pthread_rwlock_rdlock(&PayLoadMaplock);
        map<unsigned char, PAYLOADCONNECT*>::iterator iter = payload.find(head->payload_id);
        pthread_rwlock_unlock(&PayLoadMaplock);
        if (iter != payload.end())
        {
            LOGPRINT(DEBUG, "send to payload [%x] module \n", head->payload_id);
            for(;sendlen < (head->packet_length + 9); sendlen += ret)
            {
                ret = write(iter->second->fd, sp + sendlen, head->packet_length + 9 - sendlen);
                if(ret < 0)
                {
                    if(errno == EBADF || errno == EPIPE)
                    {
                        //delete iter->second;
                        LOGPRINT(DEBUG, "the payload [%x] module is be closed \n", head->payload_id);
                        break;
                    }
                    else if(errno == EAGAIN || errno == EINTR)
                    {
                        ret = 0;
                        usleep(1000);
                        LOGPRINT(DEBUG, "errno = %d, continue\n", errno);
                        continue;
                    }
                    else
                    {
                        LOGPRINT(DEBUG, " send payload [%x] module  errno = %d\n", head->payload_id, errno);
                        break;//
                    }
                }
                else if(ret == 0)
                {
                    LOGPRINT(DEBUG, "write to plat return zero\n");
                }
            }
            LOGWRITEHEX(DEBUG, sp, head->packet_length+9);
        }
        else
        {
            LOGPRINT(DEBUG, "the payload [%x] module is not connect \n", head->payload_id);
        }

        free(sp);
        pthread_mutex_lock(&QueueSockRecv);
        SockRecv.pop();
        pthread_mutex_unlock(&QueueSockRecv);
    }
    return;
}

void preServer::unixrecv()
{
    int nfds;
    for (;;)
    {
        nfds = epoll_wait(epollunfd, unevents, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            if (errno == EINTR) /* Interrupted system call */
            {
                LOGPRINT(DEBUG, "Interrupted system call");
                continue;
            }

            int_error("epoll_pwait");
            exit(EXIT_FAILURE);
        }
        for (int n = 0; n < nfds; ++n)
        {
            if (unevents[n].data.ptr == NULL)
            {
                conn_unixsock = accept(listen_unixsock, NULL, NULL);
                if (conn_unixsock == -1)
                {
                    if ((errno==EAGAIN) || (errno==EINTR))
                    {
                        usleep(1000);
                        LOGPRINT(DEBUG, "unix accept Try again \n");
                        continue;
                    }
                    int_error("accept listen_unixsock err");
                    exit(EXIT_FAILURE);
                }
                fcntl(conn_unixsock, F_SETFL, O_NONBLOCK);
                PAYLOADCONNECT *pc = new PAYLOADCONNECT();
                pc->fd = conn_unixsock;
                unev.events = EPOLLIN;
                unev.data.ptr = pc;
                if (epoll_ctl(epollunfd, EPOLL_CTL_ADD, conn_unixsock, &unev) == -1)
                {
                    int_error("epoll_ctl: conn_unixsock");
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                unix_process(unevents[n].data.ptr);
            }
        }
    }
}

void preServer::unixsend()
{
    char *sp;
    char caCinema[8+1];
    PACKHEAD* head;
    int sendlen, ret, isemRet;
    SSL* preSendCinema = NULL;
    unsigned char status;
    while(1)
    {
        if(UnixSockRecv.empty())
        {
            usleep(100);
            continue;
        }
        //isemRet = sem_wait(&m_UnixRecvSem);
        //if (isemRet == -1 && errno == EINTR)
        //{
        //    continue;
        //}

        status = 0;
        sendlen = 0;
        ret = 0;
        sp = UnixSockRecv.front();
        head = (PACKHEAD*)(sp+9);

        memset(caCinema, 0x00, sizeof(caCinema));
        memcpy(caCinema, sp, 8);
        string scinema = caCinema;

        LOGPRINT(DEBUG, "send to cinema [%s]  \n", caCinema);
        for(;sendlen<head->packet_length;sendlen += ret)
        {
            preSendCinema = NULL;
            pthread_rwlock_rdlock(&CinemaMapLock);
                map<string, CINEMACONNECT*>::iterator iter = cinema.find(scinema);
                if (iter != cinema.end())
                {
                    LOGPRINT(DEBUG, "find cinema [%s] success", caCinema);
                    preSendCinema = iter->second->ssl;
                }
            pthread_rwlock_unlock(&CinemaMapLock);

            //if (iter != cinema.end())
            if(preSendCinema != NULL)
            {
                ret = SSL_write(preSendCinema, sp+9, head->packet_length);
            }
            else
            {
                LOGPRINT(DEBUG, "the cinema [%s] module is not connect \n", caCinema);
                status = 1;
                break;
            }

            if(ret < 0)
            {
                LOGPRINT(DEBUG, "send to the cinema [%s] return [%d]. errno = [%d], \nalready send len = [%d]", caCinema, ret, errno, sendlen);
                errno = 0;
                SSLCHK(iter->second->ssl,ret);     // 这个宏会修改 ret
                if(ret == 1)
                {
                    ret = 0;
                    continue;
                }
                status = 2;
                break;
            }
            else if(ret == 0)
            {
                LOGPRINT(DEBUG, "the SSL connect is be Closed\n");
                epoll_ctl(epollfd, EPOLL_CTL_DEL, iter->second->fd, NULL);
                status = 3;
                break;
            }
        }
        LOGWRITEHEX(DEBUG, sp+9, head->packet_length);

        pthread_mutex_lock(&QueueUnixSockRecv);
        UnixSockRecv.pop();
        pthread_mutex_unlock(&QueueUnixSockRecv);

        switch(sp[9+5])
        {
        case 4:    //4
        case 11:   //B
        case 14:   //E
        case 20:   //14
        case 22:   //16
        case 25:   //19
        case 29:   //1D
        case 32:   //20
        case 37:   //25
            sp[9] = status;
            pthread_mutex_lock(&QueueqTaskRecord);
                qTaskRecord.push(sp);   // 插入前可以检查队列深度.过大则丢弃该报文.
            pthread_mutex_unlock(&QueueqTaskRecord);
            break;
        default:
            free(sp);
            break;
        }

    }
    return;
}



void *preServer::TaskRecord(void* arg)
{
    char *sp = NULL;
    char caCinema[8+1];
    unsigned char payLoad;
    unsigned char status;
    char sql[512];
    while(1)
    {
        if(qTaskRecord.empty())
        {
            usleep(100);
            continue;
        }
        LOGPRINT(DEBUG, "================> TaskRecord <================");
        sp = qTaskRecord.front();

        memset(caCinema, 0x00, sizeof(caCinema));
        memset(sql, 0x00, sizeof(sql));
        memcpy(caCinema, sp, 8);
        payLoad = sp[9+5];
        status = sp[9];
        PACKHEAD* head = (PACKHEAD* )(sp+9);
        LOGPRINT(DEBUG, "[%s][%x][%x]", caCinema, payLoad, status);
        sprintf(sql, "INSERT INTO T_M_TASK_SEND_RECORD (CINEMA,PAYLOAD,STATUS, TASK_ID) VALUES ('%s','%x','%d','%s')", caCinema, payLoad, status, sp+9+(head->packet_length));
        try
        {
            //LOGPRINT(DEBUG, sql);
            DbHelper helper;
            helper.DoExecute(sql, true);
        }
        catch(SQLException &e)
        {
        }
        catch(exception &e)
        {
        }

        free(sp);
        pthread_mutex_lock(&QueueqTaskRecord);
        qTaskRecord.pop();
        pthread_mutex_unlock(&QueueqTaskRecord);
    }
    return NULL;
}

