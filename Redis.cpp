#include "Redis.h"
#include "BaseLog.h"
#include "Config.h"

Redis::Redis()
{
    struct timeval timeout = {1, 500000}; // 1.5 seconds
    conn = redisConnectWithTimeout((char*)"127.0.0.1", 6379, timeout);
    if (conn->err) 
    {
        LOGPRINT(DEBUG, "Connection error [%s]\n", conn->errstr);
    }
    reply = NULL;
}

Redis::Redis(string& Ip, unsigned short port)
{
    struct timeval timeout = {1, 500000}; // 1.5 seconds
    conn = redisConnectWithTimeout(Ip.c_str(), port, timeout);
    if (conn->err) 
    {
        LOGPRINT(DEBUG, "Connection error %s: %d[%s]\n", Ip.c_str(), port, conn->errstr);
        exit(0);
    }
    reply = NULL;
}

Redis::~Redis()
{
    if(!conn->err)
        redisFree(conn);
    //freeReplyObject(reply);
}

int Redis::ChkConn()
{
    return conn->err;
}

int Redis::ChkListLen(string &List)
{
    int num = 0;
    reply = (redisReply *)redisCommand(conn,"LLEN %s", List.c_str());
    if (reply->type == REDIS_REPLY_INTEGER) 
    {
        num = reply->integer;
        //LOGPRINT(DEBUG, "list %s len = %d", List.c_str(), num);
    }
    freeReplyObject(reply);
    return num;
}

char* Redis::RpopList(string &List, char** Value, unsigned int* len)
{
    *len = 0;
    if(ChkListLen(List) == 0)
    {
        //LOGPRINT(DEBUG, "list is empty ");
        return NULL;
    }

    reply = (redisReply *)redisCommand(conn,"RPOP %s", List.c_str());
    if(reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_NIL)
    {
        /// err
        LOGPRINT(DEBUG, "REDIS_REPLY_ERROR || REDIS_REPLY_NIL");
    }
    else if(reply->type == REDIS_REPLY_STRING)
    {
        //LOGPRINT(DEBUG, "REDIS_REPLY_STRING len = %d", reply->len);
        //LOGWRITEHEX(DEBUG, reply->str, reply->len);
        *Value = (char*)malloc(reply->len);
        memcpy(*Value, reply->str, reply->len);
        *len = reply->len;
        //LOGWRITEHEX(DEBUG, *Value, *len);
    }
    else if(reply->type == REDIS_REPLY_ARRAY)
    {
        LOGPRINT(DEBUG, "REDIS_REPLY_ARRAY");
        LOGWRITEHEX(DEBUG, reply->element[0]->str, reply->element[0]->len);
        *Value = (char*)malloc(reply->element[0]->len);
        memcpy(*Value, reply->element[0]->str, reply->element[0]->len);
        *len = reply->len;
        LOGWRITEHEX(DEBUG, *Value, *len);
    }
    else
    {
        /// other wrong
        LOGPRINT(DEBUG, "other wrong");
    }
    freeReplyObject(reply);
    return *Value;
}

void* Redis::LpushList(string &List, char* Value, unsigned int len)
{
    reply = (redisReply *)redisCommand(conn,"LPUSH %s %b", List.c_str(), Value, len);
    if(reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_NIL)
    {
        //// err
        return (void*) reply;
    }
    else if(reply->type == REDIS_REPLY_INTEGER)
    {
        /// yes
    }
    else
    {
        /// other wrong
        return (void*) reply;
    }
    freeReplyObject(reply);
    return NULL;
}

void Redis::SetKey(const char* key, const char* value)
{
    reply = (redisReply *)redisCommand(conn,"SET %s %s", key, value);
    LOGPRINT(DEBUG, "redisCommand: SET [%s] [%s] return [%s]\n", key, value, reply->str);
    freeReplyObject(reply);
}

void Redis::SetKey(const char* key, const char* value, long time)
{
    reply = (redisReply *)redisCommand(conn,"SET %s %s", key, value);
    LOGPRINT(DEBUG, "redisCommand: SET [%s] [%s] return [%s]\n", key, value, reply->str);
    freeReplyObject(reply);
    reply = (redisReply *)redisCommand(conn,"EXPIRE %s %ld", key, time);
    LOGPRINT(DEBUG, "redisCommand: EXPIRE [%s] [%ld] return [%s]\n", key, time, reply->str);
    freeReplyObject(reply);
}

/// 更新有效时间
void Redis::UpdateKeyTime(const char* key, long time)
{
    reply = (redisReply *)redisCommand(conn,"EXPIRE %s %ld", key, time);
    LOGPRINT(DEBUG, "redisCommand: EXPIRE [%s] [%ld] return [%s]\n", key, time, reply->str);
    freeReplyObject(reply);
}

char* Redis::GetValue(const char* key, char* value, int vLen)
{
    reply = (redisReply *)redisCommand(conn,"GET %s", key);
    if(reply->type == REDIS_REPLY_ERROR || reply->type == REDIS_REPLY_NIL)
    {
        //// err
    }
    else if(reply->type == REDIS_REPLY_STRING)
    {
        strncpy(value, reply->str, vLen > (reply->len) ? (reply->len) : vLen );
        LOGPRINT(DEBUG, "redisCommand: GET [%s] return [%s]\n", key, reply->str);
    }
    else if(reply->type == REDIS_REPLY_ARRAY)
    {
        /// never in here
    }
    else
    {
        /// other wrong
    }

    freeReplyObject(reply);

    return value;
}
