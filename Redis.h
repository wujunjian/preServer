#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "hiredis.h"

using namespace std;

class Redis
{
public:
    Redis();
    Redis(string& Ip, unsigned short port);
    ~Redis();
    
    int ChkConn();
    char* RpopList(string &List, char** Value, unsigned int* len);
    void* LpushList(string &List, char* Value, unsigned int len);
    void SetKey(const char* key, const char* value);
    void SetKey(const char* key, const char* value, long time);
    void UpdateKeyTime(const char* key, long time);
    char* GetValue(const char* key, char* value, int vLen);

private:
    redisContext *conn;
    redisReply *reply;
    struct timeval timeout;

    int ChkListLen(string &List);

    //op_log_queue    local_op_log_queue    影院操作日志采集  
    //plan_queue      local_plan_queue      影院计划采集   	  
    //st_data_queue   local_st_data_queue   影院统计数据采集  
    //raw_data_queue  local_raw_data_queue  影院原始数据采集  
    //screen_queue    local_screen_queue    影厅信息采集      
    //seat_queue      local_seat_queue      影厅座位采集      
    //soft_v_queue    local_soft_v_queue    软件版本采集     
    //delay_queue     local_delay_queue
    //inform_queue    local_inform_queue
};

