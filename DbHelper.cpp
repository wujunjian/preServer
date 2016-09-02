#include "DbHelper.h"
#include <iostream>

DbHelper::DbHelper()
{
  m_Conn = NULL;
  m_Stmt = NULL;
  m_Rs = NULL;
}


DbHelper::~DbHelper(void)
{
  try
  {
    if (m_Rs != NULL)
    {
      m_Stmt->closeResultSet(m_Rs);
      m_Rs = NULL;
    }
    
    if (m_Stmt != NULL)
    {
      m_Conn->terminateStatement(m_Stmt);
      m_Stmt = NULL;
    }

    if (m_Conn != NULL)
    {
      DbPool::instance()->ReturnConn(m_Conn);
      m_Conn = NULL;
    }
  }
  catch (SQLException& ex)
  {
  }
  catch (exception& e)
  {
  }
}

Connection *DbHelper::GetConn(int timeout)
{
    if (m_Conn == NULL)
    {
        m_Conn = DbPool::instance()->BorrowConn(timeout);
    }
    return m_Conn;
}

ErrorCode DbHelper::DoQuery(string sql, int timeout)
{
  LOGPRINT(DEBUG, "try to DoQuery[%s]\n",sql.c_str());
    if (GetConn() == NULL)
    {
        LOGWRITE(DEBUG) << "noconnection" << endl;
        return ERROR_DATABASE_NOCONN;
    }
    try
    {
        LOGWRITE(DEBUG) << "Getconnection" << endl;
        m_Stmt = m_Conn->createStatement(sql);
        m_Rs = m_Stmt->executeQuery();
    }
    catch (SQLException& ex)
    {
        DbPool::instance()->JudgeSQLException(ex);
        return ERROR_DATABASE_EXECUTE;
    }
    catch (exception& e)
    {
        LOGWRITE(DEBUG) << "DoQuery exception" << endl;
        return ERROR_DATABASE_EXECUTE;
    }
    return ERROR_OK;
}

unsigned long DbHelper::GetSeq(string seqname)
{
  static unsigned long s_Seq = 1;
  unsigned long seq = s_Seq++;
  // TODO 从数据库获取
  string sql = "SELECT P_T_TICKET_DELAY_REFUND_SEQ.NEXTVAL FROM DUAL";
  try
  {
    DoQuery(sql, 5000);
    if (m_Rs != NULL && m_Rs->next())
    {
      seq = m_Rs->getInt(1);
    }
  }
  catch (SQLException &ex)
  {
  }
  catch (exception& e)
  {
  }
  return seq;
}

ErrorCode DbHelper::DoExecute(string sql, bool bAutoCommit, int timeout)
{
    LOGWRITE(INFO) << "try to DoExecute[" << sql.c_str() << "]" << endl; 

  if (GetConn() == NULL)
  {
    LOGWRITE(ERROR) << "DoExecute noconnection [" << sql.c_str() << "]" << endl; 
    return ERROR_DATABASE_NOCONN;
  }
  try
  {
    m_Stmt = m_Conn->createStatement(sql);
    m_Stmt->setAutoCommit(bAutoCommit);
    oracle::occi::Statement::Status status = m_Stmt->execute();
  }
  catch (SQLException& ex)
  {
    LOGWRITE(ERROR) << "DoExecute [" << sql.c_str() << "] SQLException : " << ex.getErrorCode()
                        << "[" << ex.getMessage().c_str() << "]" << endl; 
    return ERROR_DATABASE_EXECUTE;
  }
  catch (exception& e)
  {
    LOGWRITE(ERROR) << "DoExecute [" << sql.c_str() << "] exception "<< endl; 
    return ERROR_DATABASE_EXECUTE;
  }

  return ERROR_OK;
}

ErrorCode DbHelper::PrepareSql(string tabname, char *fields[], int fldcount, int timeout)
{
  if (GetConn() == NULL)
  {
    LOGWRITE(ERROR) << "PrepareSql [" << tabname.c_str() << "] noconnection" << endl;
    return ERROR_DATABASE_NOCONN;
  }

  try
  {
    char buf[32];
    string part1, part2;
    string sep = "";
    for (int i = 0; i < fldcount; i++)
    {
      part1 += sep + fields[i];
      sprintf(buf, "%d", i + 1);
      part2 += sep + ":" + buf;
      sep = ",";
    }
    string sql = "INSERT INTO " + tabname + "(" + part1 + ") VALUES(" + part2 + ");";
    m_Stmt = m_Conn->createStatement(sql);
  }
  catch (SQLException& ex)
  {
    return ERROR_DATABASE_EXECUTE;
  }
  catch (exception& e)
  {
    return ERROR_DATABASE_EXECUTE;
  }
  return ERROR_OK;
}
