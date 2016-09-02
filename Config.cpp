#include "Config.h"

Config *Config::s_Instance = NULL;

Config::Config(void)
{
}


Config::~Config(void)
{
}

int Config::Load(string filename)
{
  if (!m_IniParser.open(filename.c_str())) return -1;
  
  ServerIP = GetParamStringValue("ServerIP", "Server");
  ListenPort = GetParamIntegerValue("ListenPort", "Server", 8000);
  UnixSockFile = GetParamStringValue("UnixSockFile", "Server");
  Unixport = GetParamIntegerValue("Unixport", "Server", 0);
  CaCertPath = GetParamStringValue("CaCert", "Server");          ///  CA 证书
  ServerCertPath = GetParamStringValue("ServerCert", "Server");  /// 服务器证书文件
  PriKeyPath = GetParamStringValue("PriKey", "Server");          /// 私钥文件
  
    RedisIp = GetParamStringValue("RedisIp", "Redis");               /// redis服务器
    RedisPort = GetParamIntegerValue("RedisPort", "Redis", 6379);;   /// redis 端口
  
  LogSize = GetParamIntegerValue("LogSize", "Log", 1024*1024*1024);
  LogCut = GetParamIntegerValue("LogCut", "Log", 0);
  LogLevel = GetParamIntegerValue("LogLevel", "Log", 0);

  AccessIP = GetParamStringValue("AccessIP", "AccessServer");
  AccessPort = GetParamIntegerValue("AccessPort", "AccessServer", 8001);

  DbSID = GetParamStringValue("DbSID", "DbPool").c_str();
  DbUser = GetParamStringValue("DbUser", "DbPool").c_str();
  DbPwd = GetParamStringValue("DbPwd", "DbPool").c_str();
  DbPoolMin = GetParamIntegerValue("MinCount", "DbPool", 5);
  DbPoolMax = GetParamIntegerValue("MaxCount", "DbPool", 100);
  DbincrConn = GetParamIntegerValue("incrConn", "DbPool", 2);
  
  MaxRecvListSize = GetParamIntegerValue("MaxRecvListSize", "Server", 10000);
  BusinessThreadCount = GetParamIntegerValue("BusinessThreadCount", "Server", 10);
  TaskThreadCount = GetParamIntegerValue("TaskThreadCount", "Server", 0);
  Payloads = GetParamStringValue("Payloads", "Server");
  TaskPayloads = GetParamStringValue("TaskPayloads", "Server"); 
  return 0;
}

Config *Config::instance()
{
  if (s_Instance == NULL)
  {
    s_Instance = new Config;
  }
  return s_Instance;
}

string Config::GetParamStringValue(const char *name, const char* path)
{
  return m_IniParser.read(path, name);
}
int Config::GetParamIntegerValue(const char* name, const char* path, int defvalue)
{
  string value = m_IniParser.read(path, name);
  if (value.empty()) return defvalue;
  return atoi(value.c_str());
}
