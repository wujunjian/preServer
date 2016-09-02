#pragma once
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <list>
#include <queue>
#include <map>

using namespace std;

class BroadCast
{
private:
    /**
     * TaskID 任务id
     * Ip     任务分发的ip
     * Status 任务状态
    **/
    static void* UpTaskStatus(string TaskID, string Ip, int Status);
    static map<string, long> timed;

    /**
     * 判断是否可更新 
     * TaskID 任务ID
     * flag   0-检查 1-清除
    **/
    static bool CheckTime(string TaskID, int flag = 0);
public:
    static map<string, long> InitTimed();
    static void SendBroadCast(const char* msg);
    static void* RecvBroadCast(void* arg);
    static void* InsertDataBase(void* arg);
    
    static void* distributeMsg(void* arg);

    static queue<char*> BroadCaseMsg; //收到的广播消息
};
