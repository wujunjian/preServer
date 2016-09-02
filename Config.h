#pragma once
#include <string>
#include <stdexcept>
#include "IniConfig.h"
using namespace std;

class Config
{
public:
  ~Config(void);
  
  static Config *instance();
  int Load(string filepath);
public:
    string ServerIP;            /// 本机服务器ip
    unsigned short ListenPort;  /// 服务监听端口
    string UnixSockFile;        /// unixsocket file
    unsigned short Unixport;    /// 业务模块监听端口
    string RedisIp;             /// redis服务器
    unsigned short RedisPort;   /// redis 端口
    string CaCertPath;          ///  CA 证书
    string ServerCertPath;      /// 服务器证书文件
    string PriKeyPath;          /// 私钥文件
    
    long LogSize;               /// 日志大小
    int LogCut;                 /// 二进制日志是否折行, 0 不折 , 非0 折行
    int LogLevel;               /// 日志级别
    
  string AccessIP;            /// 接入服务器IP
  unsigned short AccessPort;  /// 接入服务器端口

  string DbUser;              /// 数据库用户名
  string DbPwd;               /// 数据库密码
  string DbSID;               /// 数据库SID
  int DbPoolMin;              /// 数据库最小连接数
  int DbPoolMax;              /// 数据库最大连接数
  int DbincrConn;             /// 

  int MaxRecvListSize;        /// 最大接收队列数量
  int TaskThreadCount;        /// 服务器主动任务线程数量
  int BusinessThreadCount;    /// 业务处理线程数量

  int AuthWaitSeconds;        /// 连接建立后等待登录时间
  string Payloads;            /// 用于服务的payload
  string TaskPayloads;        /// 用于主动获取任务的payload
private:
  Config(void);
  string GetParamStringValue(const char *name, const char* path = "");
  int GetParamIntegerValue(const char* name, const char* path = "", int defvalue = 0); 
  static Config *s_Instance;
  CHandle_IniFile m_IniParser;
};

