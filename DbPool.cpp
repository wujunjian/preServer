#include "DbPool.h"
#include <iostream>
#include <exception>

DbPool * DbPool::s_Instance = NULL;

DbPool::DbPool(void)
{
  m_MinConnection = 0;
  m_MaxConnection = 0;
  connPool = NULL;
  m_Env = NULL;
}

DbPool::~DbPool(void)
{
    //m_Env->terminateConnectionPool(connPool);
    m_Env->terminateStatelessConnectionPool(connPool);
    m_Env->terminateEnvironment(m_Env);
}

int DbPool::ReConn()
{
    LOGPRINT(DEBUG, "reConnect to dbpool...\n");
    try
    {
        m_Env->terminateStatelessConnectionPool(connPool);
        m_Env->terminateEnvironment(m_Env);
    
        m_Env = Environment::createEnvironment("ZHS16GBK", "ZHS16GBK", 
                Environment::Mode((Environment::OBJECT|Environment::THREADED_MUTEXED)));
                
        StatelessConnectionPool::BusyOption BusyOption=StatelessConnectionPool::NOWAIT;
        StatelessConnectionPool::PoolType pType=StatelessConnectionPool::HOMOGENEOUS; //HETEROGENEOUS
        connPool = m_Env->createStatelessConnectionPool(m_DbUser, m_DbPwd, m_DbConnStr, 
                                                        m_MaxConnection, m_MinConnection, m_IncrConnection, pType);
        connPool->setBusyOption(BusyOption);
        PrintConnectionPool();
    }
    catch(SQLException& ex) //异常处理
    {
            switch(ex.getErrorCode())
            {
                case 12541: //no listener
                case 1017:  //logon denied
                case 12514: //invalid descriptor
                    LOGWRITE(ERROR) << "SQLException Error Number :" << ex.getErrorCode() 
                                << "\n\tError Message :" << ex.getMessage() 
                                << "\n\tProgram exit" << endl;
                    exit(0);
    
                default:
                    LOGWRITE(ERROR) << "SQLException Error Number :" << ex.getErrorCode() 
                                << "\n\tError Message :" << ex.getMessage() << endl;
                    exit(0);
            }
    }
    catch (exception& e)
    {
        LOGPRINT(DEBUG, "e.what() : %s\n", e.what());
    }
    
    LOGPRINT(DEBUG, "reConnect to dbpool success...\n");
}

int DbPool::Init(string dbuser, string dbpwd, string dbconnstr, int mincount, int maxcount, int incrConn)
{
  try
  {
#if CONNPOOL
    LOGPRINT(DEBUG, "USING std::list<Connection *> for connect...\n");
    while (m_ConnList.size() > 0)
    {
      oracle::occi::Connection *conn = m_ConnList.front();
      m_Env->terminateConnection(conn);
    }
#else
    LOGPRINT(DEBUG, "USING ConnectionPool* for connect...\n");
    if(connPool != NULL)
    {
        connPool->setPoolSize(mincount, maxcount, incrConn);
        return 0;
    }
#endif

    m_MinConnection = mincount;
    m_MaxConnection = maxcount;
    m_IncrConnection = incrConn;
    m_DbUser = dbuser;
    m_DbPwd = dbpwd;
    m_DbConnStr = dbconnstr;
    // begin create connection
    m_Env = Environment::createEnvironment("ZHS16GBK", "ZHS16GBK", 
            Environment::Mode((Environment::OBJECT|Environment::THREADED_MUTEXED)));
#if CONNPOOL
    for (int i = 0; i < m_MinConnection; i++)
    {
      Connection *conn = m_Env->createConnection(m_DbUser, m_DbPwd, m_DbConnStr); 
      if (conn != NULL)
      {
        m_ConnList.push_back(conn);
      }
    }
#else
    //connPool = m_Env->createConnectionPool(m_DbUser, m_DbPwd, m_DbConnStr, mincount, maxcount, incrConn);
    StatelessConnectionPool::BusyOption BusyOption=StatelessConnectionPool::NOWAIT;
    StatelessConnectionPool::PoolType pType=StatelessConnectionPool::HOMOGENEOUS; //HETEROGENEOUS
    connPool = m_Env->createStatelessConnectionPool(m_DbUser, m_DbPwd, m_DbConnStr, 
                                                    maxcount, mincount, incrConn, pType);
    connPool->setBusyOption(BusyOption);
    //connPool->setTimeOut(5);
    LOGPRINT(DEBUG, "mincount = %d, maxcount = %d, incrConn = %d\n", mincount, maxcount, incrConn);
    PrintConnectionPool();
#endif
  }
  catch(SQLException& ex) //异常处理
  {
        switch(ex.getErrorCode())
        {
            case 12541: //no listener
            case 1017:  //logon denied
            case 12514: //invalid descriptor
                LOGWRITE(ERROR) << "SQLException Error Number :" << ex.getErrorCode() 
                            << "\n\tError Message :" << ex.getMessage() 
                            << "\n\tProgram exit" << endl;
                exit(0);

            default:
                LOGWRITE(ERROR) << " SQLException Error Number :" << ex.getErrorCode() 
                            << "\n\tError Message :" << ex.getMessage() << endl;
                exit(0);
        }
  }
  catch (exception& e)
  {
  }
  if (m_ConnList.size() != mincount) return false;
  return 0;
}

Connection *DbPool::BorrowConn(int ms)
{
    Connection *pool;
#if CONNPOOL
    pool = m_ConnList.size() == 0 ? NULL : m_ConnList.front();
    if (pool != NULL) m_ConnList.pop_front();
#else
    try
    {
        //pool = connPool->createConnection(m_DbUser, m_DbPwd);
        //pool = connPool->createProxyConnection(m_DbUser);
        pool = connPool->getConnection();
    }
    catch(SQLException& ex)
    {
        LOGPRINT(DEBUG, "SQLException Error Number : %d,Error Message : %s \n", ex.getErrorCode(), 
                        ex.getMessage().c_str());
        return NULL;
    }
#endif
    return pool;
}

void DbPool::ReturnConn(Connection *conn)
{
#if CONNPOOL
  m_ConnList.push_back(conn);
#else
    //connPool->terminateConnection(conn);
    connPool->releaseConnection(conn);
#endif
}

Connection *DbPool::AfreshConn(Connection *conn)  //刷新不可用链接
{
    try{
#if CONNPOOL
        m_Env->terminateConnection(conn);
        conn = m_Env->createConnection(m_DbUser, m_DbPwd, m_DbConnStr);
#endif
    }
    catch(SQLException& ex)
    {
        LOGWRITE(ERROR) << "该异常未做处理. SQLException Error Number :" << ex.getErrorCode() 
                            << "\n\tError Message :" << ex.getMessage() << endl;
    }

    return conn;

}

int DbPool::JudgeSQLException(SQLException &ex)
{
    switch(ex.getErrorCode())
    {
        case 12541: //no listener
        case 1017:  //logon denied
        case 12514: //invalid descriptor
            LOGWRITE(ERROR) << "SQLException Error Number :" << ex.getErrorCode() 
                        << "\n\tError Message :" << ex.getMessage() 
                        << "\n\tProgram exit" << endl;
            exit(0);
        case 3135:  //connection lost contact
		case 1012:  //not logged on
		case 28:	// your session has been killed
            LOGWRITE(DEBUG) << "SQLException : " << ex.getErrorCode()
                        << "[" << ex.getMessage().c_str() << "]" << endl;
			ReConn();
            break;
        default:
            LOGWRITE(ERROR) << "该异常未做处理. SQLException Error Number :" << ex.getErrorCode() 
                        << "\n\tError Message :" << ex.getMessage() << endl;      
            exit(0);
    }
    return ex.getErrorCode();
}

void DbPool::PrintConnectionPool()
{
    LOGPRINT(STRICT, "\nConnectionPool[%s]:\nTimeOut:[%d]\n"
                    "MinConnections:[%d]\nMaxConnections:[%d]\n"
                    "BusyConnections:[%d]\nOpenConnections:[%d]\n",
                    connPool->getPoolName().c_str(),
                    connPool->getTimeOut(),
                    connPool->getMinConnections(),
                    connPool->getMaxConnections(),
                    connPool->getBusyConnections(),
                    connPool->getOpenConnections());
}


