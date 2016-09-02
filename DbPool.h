#pragma once
#include <occi.h>
#include <list>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include "BaseLog.h"
using namespace std;
using namespace oracle::occi;

#define CONNPOOL 0

class DbPool
{
public:
  int Init(string dbuser, string dbpwd, string dbconnstr, int mincount, int maxcount, int incrConn);
  int ReConn();
  Connection *BorrowConn(int ms);
  Connection *AfreshConn(Connection *conn);
  void ReturnConn(Connection *conn);
  int JudgeSQLException(SQLException &ex);
  void PrintConnectionPool();
  static DbPool *instance(){if (s_Instance == NULL) s_Instance = new DbPool;return s_Instance;}
private:
  DbPool();
  ~DbPool();
  Environment *m_Env;
  int m_MinConnection;
  int m_MaxConnection;
  int m_IncrConnection;
  int m_MonitorTime;
  std::list<Connection *> m_ConnList;
  //ConnectionPool *connPool;
  StatelessConnectionPool *connPool;
  string m_DbUser;
  string m_DbPwd;
  string m_DbConnStr;
  static DbPool *s_Instance;
};

