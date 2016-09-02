#pragma once
#include <string.h>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <list>
#include <map>
#include <stdarg.h>
#include <sys/time.h>

using namespace std;
using std::ofstream;

enum
{
  DEBUG = 0,
  INFO,
  ERROR,
  STRICT,
  CREATE
};

//日志文件名, 如没有提供,或在线程打印日志前没有提供,则日志文件名 like %NOLOGFILENAME%
#define LOGINIT(LOGFILENAME) BaseLog::instance()->SetFile(LOGFILENAME)

//流输出,没有日志级别控制
#define LOGWRITE(LEVEL) *(BaseLog::instance()->GetFile()) \
                         << __FILE__ << ":" << __LINE__ << ":"#LEVEL"]: "

//ASCII码对照
#define LOGWRITEHEX(LEVEL, BUF, LEN) BaseLog::instance()->MemDump(LEVEL, (unsigned char*)BUF, LEN, Config::instance()->LogCut)

//按日志级别,格式化打印
#define LOGPRINT(LEVEL, format, ...) if(LEVEL >= BaseLog::instance()->l_ServerLevel)\
                                    {char Str[1024]; snprintf(Str, 1024, format, ##__VA_ARGS__); LOGWRITE(LEVEL) << Str << endl;}
#define LOGOFARGS_1(format, args...) fprintf(stdout, format, ##args)
#define SQLWRITE(FILENAME) *(BaseLog::instance()->GetSqlFile(FILENAME))

class BaseLog
{
public:
    BaseLog();
    ~BaseLog();
    static BaseLog *instance();
    ofstream* GetFile();
    ofstream* GetSqlFile(string FileName);
    void SetFile(string FileName);
    void MemDump(int level, unsigned char *pucaAddr, int lLength, int cut = 0);
    void ChgLogLevel(int level);        //改变日志打印级别
    BaseLog& operator<<(const string &str);
    pthread_mutex_t mutex;
    int l_ServerLevel;   //日志打印级别

private:
    map<long, ofstream*>l_iofile;
    map<long, string>l_iofileName;
    map<long, ofstream*>l_sqlfile;
    map<long, long>l_sqlFileTime;
    static BaseLog *l_Instance;
    string l_Time;
    
};

