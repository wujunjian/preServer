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
    string ServerIP;            /// ����������ip
    unsigned short ListenPort;  /// ��������˿�
    string UnixSockFile;        /// unixsocket file
    unsigned short Unixport;    /// ҵ��ģ������˿�
    string RedisIp;             /// redis������
    unsigned short RedisPort;   /// redis �˿�
    string CaCertPath;          ///  CA ֤��
    string ServerCertPath;      /// ������֤���ļ�
    string PriKeyPath;          /// ˽Կ�ļ�
    
    long LogSize;               /// ��־��С
    int LogCut;                 /// ��������־�Ƿ�����, 0 ���� , ��0 ����
    int LogLevel;               /// ��־����
    
  string AccessIP;            /// ���������IP
  unsigned short AccessPort;  /// ����������˿�

  string DbUser;              /// ���ݿ��û���
  string DbPwd;               /// ���ݿ�����
  string DbSID;               /// ���ݿ�SID
  int DbPoolMin;              /// ���ݿ���С������
  int DbPoolMax;              /// ���ݿ����������
  int DbincrConn;             /// 

  int MaxRecvListSize;        /// �����ն�������
  int TaskThreadCount;        /// ���������������߳�����
  int BusinessThreadCount;    /// ҵ�����߳�����

  int AuthWaitSeconds;        /// ���ӽ�����ȴ���¼ʱ��
  string Payloads;            /// ���ڷ����payload
  string TaskPayloads;        /// ����������ȡ�����payload
private:
  Config(void);
  string GetParamStringValue(const char *name, const char* path = "");
  int GetParamIntegerValue(const char* name, const char* path = "", int defvalue = 0); 
  static Config *s_Instance;
  CHandle_IniFile m_IniParser;
};

