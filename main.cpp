#include "preServer.h"
#include "common.h"
#include "BaseLog.h"
#include "DbPool.h"
#include "Config.h"
#include "SigMask.h"

void *TaskRecord(void* arg)
{
    LOGINIT("TaskRecord");
    preServer::instance()->TaskRecord(NULL);
}

void *recv(void* arg)
{
    LOGINIT("TCPRECV");
    preServer::instance()->recv();
    LOGPRINT(DEBUG, "this will be never returned, recv err\n");
}

void *unixrecv(void* arg)
{
    LOGINIT("UNIXRECV");
    preServer::instance()->unixrecv();
    LOGPRINT(DEBUG, "this will be never returned, unixrecv err\n");
}

void *send(void* arg)
{
    LOGINIT("SENDTOPLAT");
    preServer::instance()->send();
    LOGPRINT(DEBUG, "this will be never returned, send err\n");
}

void *unixsend(void* arg)
{
    LOGINIT("SENDTOTCP");
    preServer::instance()->unixsend();
    LOGPRINT(DEBUG, "this will be never returned, unixsend err\n");
}

int main(int argc, char** argv)
{
    LOGINIT(basename(argv[0]));
    LOGWRITE(DEBUG) << "====================> Begin <====================" << endl;

    // 读取配置文件
    int ret = Config::instance()->Load("config.ini");
    if (ret != 0)
    {
        LOGWRITE(ERROR) << "load config error..." << endl;
        return 1;
    }

    // 数据库连接池
    ret = DbPool::instance()->Init(Config::instance()->DbUser, Config::instance()->DbPwd, Config::instance()->DbSID, 
    Config::instance()->DbPoolMin, Config::instance()->DbPoolMax, Config::instance()->DbincrConn);
    if (ret != 0)
    {
        LOGWRITE(ERROR) << "DbPool create error..." << endl;
        return 1;    
    }

    BaseLog::instance()->ChgLogLevel(Config::instance()->LogLevel);

    THREAD_TYPE recv_tid, unixrecv_tid, send_tid, unixsend_tid, TaskRecord_tid;
    preServer::instance()->Init();

    THREAD_CREATE(recv_tid,recv,NULL);
    THREAD_CREATE(unixrecv_tid,unixrecv,NULL);
    THREAD_CREATE(send_tid,send,NULL);
    THREAD_CREATE(unixsend_tid,unixsend,NULL);
    THREAD_CREATE(TaskRecord_tid,TaskRecord,NULL);
    

	SigMask::instance()->Register();
    while(1)
        pause();
	return 0;
}


