#include <iostream>
#include <list>
#include <queue>
#include "common.h"
#include "BaseLog.h"
#include "BroadCast.h"
#include "DbPool.h"
#include "Config.h"

int main(int argc, char** argv)
{
    LOGINIT(basename(argv[0]));
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

    BroadCast::SendBroadCast("ANOTHERONELOGIN");           /// 发送登陆广播, 其他机器上程序退出.
    sleep(2);
    THREAD_TYPE tid;
    THREAD_CREATE(tid,BroadCast::RecvBroadCast,NULL);
    THREAD_CREATE(tid,BroadCast::InsertDataBase,NULL);

    /// redis 线程
    
    while(1)
        pause();
}
