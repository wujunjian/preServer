#include "BaseLog.h"
#include "Config.h"

BaseLog *BaseLog::l_Instance = NULL;

BaseLog::BaseLog()
{
    time_t t_now = time(NULL);
    struct tm* tm_now = localtime(&t_now);
    char c_Tmp[16] = {0};
    sprintf(c_Tmp, "%d%02d%02d%02d%02d%02d",
                tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday,
                tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
    l_Time = c_Tmp;
    l_ServerLevel = 0;
    pthread_mutexattr_t mattr;
    //ret = pthread_mutexattr_init(&mattr); /* * resetting to its default value: private */
    //ret = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_PRIVATE);
    //pthread_mutexattr_settype(pthread_mutexattr_t *attr , int type);
    pthread_mutex_init(&mutex,NULL);
}

BaseLog::~BaseLog()
{
    map<long, ofstream*>::iterator iter;
    for (iter = l_iofile.begin(); iter != l_iofile.end(); iter++)
    {
        iter->second->close();
    }
    l_iofile.clear();
    
    for (iter = l_sqlfile.begin(); iter != l_sqlfile.end(); iter++)
    {
        iter->second->close();
    }
    l_sqlfile.clear();
    
    pthread_mutex_destroy(&mutex);
    //b_iofile.erase(b_iofile.begin(),b_iofile.end());
}

BaseLog *BaseLog::instance()
{

    if (l_Instance == NULL)
        l_Instance = new BaseLog();

    return l_Instance;
}

ofstream* BaseLog::GetFile()
{
    //time_t t_now = time(NULL);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm_now;
    localtime_r(&tv.tv_sec, &tm_now);
    char c_Tmp[16] = {0};
    sprintf(c_Tmp, "%d%02d%02d%02d%02d%02d.%06d",
                tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday,
                tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, tv.tv_usec);

    pthread_t tid = pthread_self();
     
    pthread_mutex_lock(&mutex);
    map<long, ofstream*>::iterator iter = l_iofile.find(tid);
    map<long, string>::iterator iterFile = l_iofileName.find(tid);
    if (iter == l_iofile.end())
    {
        char FileNameTmp[128] = {0};
        sprintf(FileNameTmp, "%d%02d%02d.%s_%s_%ld.log", tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, l_Time.c_str(), "NOLOGFILENAME", tid);
        ofstream *filetmp = new ofstream(FileNameTmp, ios::app);
        l_iofile.insert(map<long, ofstream*>::value_type(tid, filetmp));
        l_iofileName.insert(map<long, string>::value_type(tid, string(FileNameTmp)));
        pthread_mutex_unlock(&mutex);
        *filetmp << "[" << c_Tmp << ":";
        return filetmp;
    }
    else if (iter->second->tellp() > Config::instance()->LogSize || memcmp(c_Tmp, (iterFile->second).c_str(), 8) )
    {
        iter->second->close();      //关闭文件
        //remove("");
        char FileNameTmp[128] = {0};
        if(iterFile == l_iofileName.end())
        {
            sprintf(FileNameTmp, "%d%02d%02d.%s_%s_%ld.log", tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, l_Time.c_str(), "NOLOGFILENAME", tid);
        }
        else
        {
            strcpy(FileNameTmp, (iterFile->second).c_str());
            
            string OldName = string(FileNameTmp) + string(c_Tmp);
            rename(FileNameTmp, OldName.c_str()); 

            memcpy(FileNameTmp, c_Tmp, 8);
        }

        iter->second->open(FileNameTmp, ios::app);       //重新打开一个文件
        l_iofileName[tid] = string(FileNameTmp);
    }
    pthread_mutex_unlock(&mutex);
    *(iter->second) << "[" << c_Tmp << ":";
    return iter->second; 
}

ofstream* BaseLog::GetSqlFile(string FileName)
{
    time_t t_now = time(NULL);
    pthread_t tid = pthread_self();
    pthread_mutex_lock(&mutex);
    map<long, ofstream*>::iterator iter = l_sqlfile.find(tid);
    map<long,long>::iterator iter2 = l_sqlFileTime.find(tid);
    if (iter == l_sqlfile.end() || iter->second->tellp() > 1024*1024 ||
            (t_now - iter2->second) > 180)
    {
        if(iter != l_sqlfile.end())
        {
            iter->second->close();
            delete iter->second;
            l_sqlfile.erase(iter);
            l_sqlFileTime.erase(iter2);
        }

        struct tm* tm_now = localtime(&t_now);
        char c_Tmp[16] = {0};
        sprintf(c_Tmp, "%d%02d%02d%02d%02d%02d",
                tm_now->tm_year+1990, tm_now->tm_mon+1, tm_now->tm_mday,
                tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

        char FileNameTmp[128] = {0};
        sprintf(FileNameTmp, "%s_%s_%ld.sql", c_Tmp, FileName.c_str(), tid);
        ofstream *filetmp = new ofstream(FileNameTmp, ios::app);
        l_sqlfile.insert(map<long, ofstream*>::value_type(tid, filetmp));
        l_sqlFileTime.insert(map<long, long>::value_type(tid, t_now));
        pthread_mutex_unlock(&mutex);
        return filetmp;
    }
    pthread_mutex_unlock(&mutex);
    return iter->second; 
}

void BaseLog::SetFile(string FileName)
{
    time_t t_now = time(NULL);
    struct tm* tm_now = localtime(&t_now);
    pthread_t tid = pthread_self();
    pthread_mutex_lock(&mutex);
    map<long, ofstream*>::iterator iter = l_iofile.find(tid);
    if (iter == l_iofile.end())
    {
        char FileNameTmp[128] = {0};
        sprintf(FileNameTmp, "%d%02d%02d.%s_%s_%ld.log", tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, l_Time.c_str(), FileName.c_str(), tid);
        ofstream *filetmp = new ofstream(FileNameTmp, ios::app);
        l_iofile.insert(map<long, ofstream*>::value_type(tid, filetmp));
        l_iofileName.insert(map<long, string>::value_type(tid, string(FileNameTmp)));
    }
    pthread_mutex_unlock(&mutex);

    return;
}
    
void BaseLog::MemDump(int level, unsigned char *pucaAddr, int lLength, int cut)
{
    if(level < l_ServerLevel)
        return;
    int  i,j,n;
    int  iPage = 20;
    char caTmp[100];
    char caBuf[1650] = {0};
    unsigned char *pcPtr;
    ofstream* wFile = l_Instance->GetFile();

    pcPtr=pucaAddr;
    if(cut != 0)
    {
        while(pcPtr < (pucaAddr + lLength))
        {
            sprintf(caBuf+strlen(caBuf), "%02x", *pcPtr);
            pcPtr++;
        
            if(strlen(caBuf) > 1500)
            {
                *wFile << caBuf;
                caBuf[0]='\0';
            }
        }
        *wFile << caBuf << endl;
        return ;
    }
    while ( pcPtr < (pucaAddr + lLength))
    {
        for (j=0;j <= (lLength-1)/16 ; j++)
        {
            //if (j == (j/iPage)*iPage)
            if (j % iPage == 0)
            {
                strcpy(caTmp,"\nDisplacement ");
                strcat(caTmp,"-0--1--2--3--4--5--6--7--8--9--A--B--C--D--E--F");
                strcat(caTmp,"  --ASCII  Value--\n");
                *wFile << caTmp;
            }

            sprintf(caTmp, "%05d(%05x) ", j*16, j*16);
            for (i=0; (i<16)&&(pcPtr<(pucaAddr+lLength)); i++)
            {
                sprintf(caTmp+strlen(caTmp),"%02x ", *pcPtr);
                pcPtr++;
            }
            for (n=0; n<16-i; n++)
            {
                strcat(caTmp,"   ");
            }
            strcat(caTmp," ");
            pcPtr = pcPtr - i;

            for (n=0; n<i; n++)
            {
                if( (((*pcPtr)<=31) && ((*pcPtr)>=0)) || ((*pcPtr)>=127))
                {
                    strcat(caTmp,".");
                }
                else
                {
                    sprintf(caTmp+strlen(caTmp),"%c",*pcPtr);
                }
                pcPtr++;
            }

            strcat(caBuf,caTmp);
            strcat(caBuf,"\n");
            //if (j == (j/iPage)*iPage)
            if (j % iPage == iPage - 1)
            {
                *wFile << caBuf;
                caBuf[0]='\0';
            }
        } /* end of for */
    } /* end of while */

    *wFile << caBuf << endl;
} /* end of MemDump   */

void BaseLog::ChgLogLevel(int level)
{
    l_ServerLevel = level;
}

BaseLog& BaseLog::operator<<(const string &str)
{
    *(BaseLog::instance()->GetFile()) << str.c_str();
    /* do nothing */
    return *this;
}

