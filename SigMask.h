#pragma once
#include <string>
//#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>
#include <list>
#include <map>
#include <signal.h>
#include "Config.h"
#include "BaseLog.h"
#include "DbPool.h"

using namespace std;
typedef void(*FUNP)(int); 

class SigMask
{
public:
    SigMask();
    ~SigMask();
    static SigMask *instance();
    void Register();
    static void ReLoadConfig(int sig); //重读配置文件
    static void PrintConnectionPool(int sig);

private:
    void Signal(unsigned int sig, FUNP function);
    static SigMask *l_Instance;
    sigset_t sigset;
};

