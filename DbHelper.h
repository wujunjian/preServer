#pragma once
#include <cstdio>
#include <string>
using namespace std;
#include "DbPool.h"
#include "DataTypes.h"
#include "BaseLog.h"

class DbHelper
{
public:
  DbHelper();
  ~DbHelper(void);

  unsigned long GetSeq(string seqname);

  ErrorCode DoQuery(string sql, int timeout = 5000);
  ErrorCode DoExecute(string sql, bool bAutoCommit, int timeout = 5000);
  ResultSet *GetRs(){return m_Rs;}
  ErrorCode PrepareSql(string sql, char *fields[], int fldcount, int timeout = 5000);
  void SetField(int id, string fldtype, void *data);

  Statement *GetStmt(){return m_Stmt;}

  static void AddStringField(string &buf, string sep, char *value){buf += sep + "'" + value + "'";}
  static void AddIntField(string &buf, string sep, uint32 value){char data[32];sprintf(data, "%u", value);buf += sep + data;}
  static void AddLongField(string &buf, string sep, unsigned long value){char data[32];sprintf(data, "%lu", value);buf += sep + data;}
  static void AddSeqField(string &buf, string sep, string seq){buf += sep + seq + ".NEXTVAL";}
  static void AddFloatField(string &buf, string sep, float value){char data[32];sprintf(data, "%f", value);buf += sep + data;}
  static void AddDateField(string &buf, string sep, string value){buf += sep + "TO_DATE('" + value + "','YYYY-MM-DD')";}
  static void AddTimestampField(string &buf, string sep, string value){buf += sep + "TO_TIMESTAMP('" + (value.length() == 19 ? (value.substr(0, 10) + " " + value.substr(11, 8)) : value) + "','YYYY-MM-DD HH24:MI:SS')";}
  static void AddTimestampField(string &buf, string sep, time_t value){buf += sep + "SYSTIMESTAMP";}
  static void AddTimestampField(string &buf, string sep){buf += sep + "SYSTIMESTAMP";}
private:
  Connection *GetConn(int timeout = 5000);
private:
  Connection *m_Conn;
  Statement *m_Stmt;
  ResultSet *m_Rs;

};

