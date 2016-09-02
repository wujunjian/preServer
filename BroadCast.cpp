#include "BroadCast.h"
#include "BaseLog.h"
#include "Redis.h"
#include "DbHelper.h"
#include "Config.h"

queue<char*> BroadCast::BroadCaseMsg;
map<string, long> BroadCast::timed = BroadCast::InitTimed();


bool BroadCast::CheckTime(string TaskID, int flag)
{
    map<string, long>::iterator iter = timed.find(TaskID);
    if (iter != timed.end())
    {
        switch(flag)
        {
        case 0:
            if(time(NULL) - iter->second < 120)
                return false;
            iter->second = time(NULL);
            break;
        case 1:
            timed.erase(iter);
            break;
        default:
            break;
        }
    }
    else
    {
        timed.insert(map<string, long>::value_type(TaskID, time(NULL)));
    }
    return true;
}

map<string, long> BroadCast::InitTimed()
{
    map<string, long> InitTimed;
    return InitTimed;
}

/**
 * status : 1.成功 2.影院不在线 3.超时丢弃 4.redis连接失败
**/
void* BroadCast::UpTaskStatus(string TaskID, string Ip, int Status)
{
    DbHelper helper;

    char caStatus[2] = {0};
    sprintf(caStatus, "%d", Status);
    string sql =  "UPDATE T_M_TASK_EXE_CONDITIONS  SET  TASK_EXE_MACHINE = '" + Ip 
               + "', DEAL_TIME = SYSTIMESTAMP, DISTRIBUTE_STATE = '" + caStatus
               + "' WHERE TASK_EXE_CONDITIONS_ID='" + TaskID
               + "'";

    try
    {
        helper.DoExecute(sql, 5000);
    }
    catch (SQLException& ex)
    {
        LOGWRITE(ERROR) << "DoExecute [" << sql.c_str() << "] SQLException : " << ex.getErrorCode() << "[" << ex.getMessage().c_str() << "]" << endl; 
    }
    catch (exception& e)
    {  
    }
    return NULL;
}

void* BroadCast::distributeMsg(void* arg)
{
    Redis redis(Config::instance()->RedisIp, Config::instance()->RedisPort);
    if(redis.ChkConn())
    {
        return NULL;
    }
    string sListPop[] = {"op_log_queue",
                         "plan_queue",
                         "st_data_queue",
                         "raw_data_queue",
                         "screen_queue",
                         "seat_queue",
                         "soft_v_queue",
                         "delay_queue",
                         "inform_queue",
                         "0000"
                         };
    string sListPush[] = {"local_op_log_queue",
                          "local_plan_queue",
                          "local_st_data_queue",
                          "local_raw_data_queue",
                          "local_screen_queue",
                          "local_seat_queue",
                          "local_soft_v_queue",
                          "local_delay_queue",
                          "local_inform_queue",
                          "0000"
                          };

    for(int i = 0; strcmp((sListPop[i]).c_str(), "0000"); i++)
    {
        char* buf = NULL;
        char IpBuf[20] = {0};
        unsigned int len = 0;
        redis.RpopList(sListPop[i], &buf, &len);
        if(buf != NULL)
        {
            //时间戳(20) + 编码(9) + 任务id(32)
            //LOGPRINT(DEBUG, "CinemaCode = %s", buf + 20);

            memset(IpBuf, 0x00, sizeof(IpBuf));
            redis.GetValue(buf + 20, IpBuf, sizeof(IpBuf));

            //LOGPRINT(DEBUG, "Ip = %s", IpBuf);
            if(0 != strlen(IpBuf))
            {
                string IpString = IpBuf;
                Redis PeerRedis(IpString, 6379);
                if(PeerRedis.ChkConn())
                {
                    //redis 连接失败,写回队列
                    if(NULL != redis.LpushList(sListPop[i], buf, len))
                    {
                        CheckTime(string(buf+29), 1); //清除map
                        UpTaskStatus(string(buf+29), IpString, 5); //写redis队列失败
                    }
                    else
                    {
                        if(CheckTime(string(buf+29), 0))
                            UpTaskStatus(string(buf+29), IpString, 4);
                    }
                    return NULL;
                }
                CheckTime(string(buf+29), 1); //清除map
                if(NULL != PeerRedis.LpushList(sListPush[i], buf, len))
                {
                    UpTaskStatus(string(buf+29), IpString, 5); //写入redis队列失败
                }
                else
                {
                    UpTaskStatus(string(buf+29), IpString, 1); //成功
                }
            }
            else
            {
                // 检查报文时效
                struct tm tm;
                char caTmp[4+1] = {0};
                
                strncpy(caTmp, buf+17, 2);
                tm.tm_sec = atoi(caTmp);         /* seconds */
                
                strncpy(caTmp, buf+14, 2);
                tm.tm_min = atoi(caTmp);         /* minutes */
                
                strncpy(caTmp, buf+11, 2);
                tm.tm_hour = atoi(caTmp);        /* hours */
                
                strncpy(caTmp, buf+8, 2);
                tm.tm_mday = atoi(caTmp);        /* day of the month */
                
                strncpy(caTmp, buf+5, 2);
                tm.tm_mon = atoi(caTmp) - 1;         /* month */
                
                strncpy(caTmp, buf, 4);
                tm.tm_year = atoi(caTmp) - 1900;        /* year */
                
                //tm.tm_wday = atoi(caTmp);        /* day of the week */
                //tm.tm_yday = atoi(caTmp);        /* day in the year */
                tm.tm_isdst = 0;       /* daylight saving time */
                
                time_t CreateTime = mktime(&tm);
                time_t now;
                time(&now);
                if((now - CreateTime) > 3600*24)
                {
                    LOGPRINT(DEBUG, "Task is overtime now = %ld, CreateTime = %ld", now, CreateTime);
                    CheckTime(string(buf+29), 1); //清除
                    UpTaskStatus(string(buf+29), "", 3);
                }
                else
                {
                    //  影院没找到,写回原队列
                    if(NULL != redis.LpushList(sListPop[i], buf, len))
                    {
                        CheckTime(string(buf+29), 1);       //清除map
                        UpTaskStatus(string(buf+29), "", 5);//写回redis队列失败
                    }
                    else
                    {
                        if(CheckTime(string(buf+29), 0))
                            UpTaskStatus(string(buf+29), "", 2);
                    }
                }
            }
            free(buf);
            buf = NULL;
        }
        else
        {
            //LOGPRINT(DEBUG, "read zero of [%s]", sListPop[i].c_str());
        }
    }
}

void* BroadCast::InsertDataBase(void* arg)
{
    LOGINIT("INSERTDATABASE");
    while(1)
    {
        if(BroadCaseMsg.empty())
        {
            /// distribute msg
            distributeMsg((void*)NULL);
            usleep(10000);
            //sleep(2);
            continue;
        }
        
        Redis redis(Config::instance()->RedisIp, Config::instance()->RedisPort);
        if(redis.ChkConn())
        {
            usleep(10000);
            continue;
        }
        char* data = BroadCaseMsg.front();

        char* value = strstr(data, ":") + 1;
        *(value-1) = 0x00;
        char* key = data;
        if(8 != strlen(key))
        {
            redis.SetKey(key, value);
            free(data);
            BroadCaseMsg.pop();
            continue;
        }

        redis.SetKey(key, value, 310);

        DbHelper helper;
        string Cinema(key, 8);
        char Ip[20] = {0};
        strncpy(Ip,data + 9, 16);
        //Ip.find_first_of(':');
        //if(Ip.find_first_of(':') < Ip.length())
        //    Ip.replace(Ip.find_first_of(':'), 1, 0x00);
        string sql = "insert into t_m_conn_hear_beat_log "
             "(LOG_ID,CINEMA_CODE,IS_CONN, INSERT_TIME,IP, HEAR_BEAT_TIME) "
             "values (t_m_conn_hear_beat_log_seq.NEXTVAL,'" + Cinema + "', '1', sysdate, '" + Ip + "', sysdate)";

        LOGPRINT(DEBUG, "the sql is [%s]", sql.c_str());
        try
        {
            helper.DoExecute(sql, 5000);
        }
        catch (SQLException& ex)
        {
            LOGWRITE(ERROR) << "DoExecute [" << sql.c_str() << "] SQLException : " << ex.getErrorCode()
                        << "[" << ex.getMessage().c_str() << "]" << endl; 
        }
        catch (exception& e)
        {  
        }

        free(data);
        BroadCaseMsg.pop();
    }


}
//接收
void* BroadCast::RecvBroadCast(void* arg)
{
    LOGINIT("RECVBROADCAST");
    // 绑定地址
    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family = AF_INET;
    addrto.sin_addr.s_addr = htonl(INADDR_ANY);
    addrto.sin_port = htons(8002);

    // 广播地址
    struct sockaddr_in from;
    bzero(&from, sizeof(struct sockaddr_in));

    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOGPRINT(DEBUG, "the BroadCast socket err\n");
        return false;
    }

    const int opt = 1;
    //设置该套接字为广播类型，
    int nb = 0;
    nb = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if(nb == -1)
    {
        LOGPRINT(DEBUG, "set BroadCast socket err\n");
        return false;
    }

    if(bind(sock,(struct sockaddr *)&(addrto), sizeof(struct sockaddr_in)) == -1)
    {
        LOGPRINT(DEBUG, "bind BroadCast socket err\n");
        return false;
    }

    int len = sizeof(sockaddr_in);
    char smsg[1024] = {0};
    while(1)
    {
        memset(smsg, 0x00, sizeof(smsg));
        //从广播地址接受消息
        int ret=recvfrom(sock, smsg, sizeof(smsg), 0, (struct sockaddr*)&from,(socklen_t*)&len);
        if(ret<=0)
        {
            LOGPRINT(DEBUG, "read BroadCast message err\n");
        }
        else
        {
            LOGPRINT(DEBUG, "receive BroadCast message OK! [%s] \n", smsg);
            if(!strcmp(smsg, "ANOTHERONELOGIN"))
                exit(0);

            char* p = (char*)malloc(strlen(smsg)+1);
            strcpy(p, smsg);
            BroadCaseMsg.push(p);
        }

        usleep(1000);
    }

    close(sock);
    return 0;
}

//发送
void BroadCast::SendBroadCast(const char* msg)
{
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        LOGPRINT(DEBUG, "the BroadCast socket err\n");
        return;
    }

    const int opt = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt)) == -1)
    {
        LOGPRINT(DEBUG, "set BroadCast socket err\n");
        return;
    }

    struct sockaddr_in addrto;
    bzero(&addrto, sizeof(struct sockaddr_in));
    addrto.sin_family=AF_INET;
    addrto.sin_addr.s_addr=htonl(INADDR_BROADCAST);
    addrto.sin_port=htons(8002);

    int ret=sendto(sock, msg, strlen(msg), 0, (sockaddr*)&addrto, sizeof(addrto));
    if(ret<0)
    {
        LOGPRINT(DEBUG, "send BroadCast message [%s] err\n", msg);
    }
    else
    {
        LOGPRINT(DEBUG, "send BroadCast message [%s] OK! \n", msg);
    }
    
    close(sock);

    return;
}

