#include "SigMask.h"

SigMask *SigMask::l_Instance = NULL;

SigMask::SigMask()
{
    sigfillset(&sigset);
    sigdelset(&sigset, SIGALRM);
    sigdelset(&sigset, SIGUSR1);
    sigdelset(&sigset, SIGUSR2);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
    

}

SigMask::~SigMask()
{
}

SigMask *SigMask::instance()
{
    if (l_Instance == NULL)
        l_Instance = new SigMask();

    return l_Instance;
}

void SigMask::Signal(unsigned int signo, FUNP function)
{
    struct sigaction act, oldact;
    
    act.sa_handler = function;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, signo);
    act.sa_flags = SA_RESTART;
    if(sigaction(signo, &act, &oldact) == -1)
    {
        // 设置信号处理函数出错
        return;
    }
    return;
}

void SigMask::ReLoadConfig(int sig)
{
    int iRet = Config::instance()->Load("config.ini");
    if(iRet != 0)
        LOGPRINT(ERROR, "ReLoad(config.ini) err[%d]", iRet);
        
    LOGPRINT(ERROR, "ReLoad(config.ini) success[%d]", 0);
}

void SigMask::PrintConnectionPool(int sig)
{
    DbPool::instance()->PrintConnectionPool();
}

void SigMask::Register()
{
    Signal((unsigned int)SIGUSR1, ReLoadConfig);
    Signal((unsigned int)SIGUSR2, PrintConnectionPool);
    //signal((unsigned int)SIGUSR1, ReLoadConfig);
}
