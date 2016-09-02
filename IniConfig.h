#pragma once
#include <cstring>
#include <map>

#include <string>

#include <vector>

#include <algorithm>

#include <functional>

#include <fstream>

#include <iostream>
#include <iterator>


using namespace std;



typedef map<string, string, less<string> > strMap;

typedef strMap::iterator strMapIt;



const char*const MIDDLESTRING = "_____***_______";



struct analyzeini{

  string strsect;

  strMap *pmap;

  analyzeini(strMap & strmap):pmap(&strmap){}       

  void operator()( const string & strini)

  {

    int first = (int)strini.find('[');

    int last = (int)strini.rfind(']');

    if( first != string::npos && last != string::npos && first != last+1)

    {

      strsect = strini.substr(first+1,last-first-1);

      return ;

    }

    if(strsect.empty())

      return ;

    if((first=(int)strini.find('=')) == string::npos)

      return ;

    string strtmp1= strini.substr(0,first);

    string strtmp2=strini.substr(first+1, string::npos);

    first= (int)strtmp1.find_first_not_of(" \t");

    last = (int)strtmp1.find_last_not_of(" \t");

    if(first == string::npos || last == string::npos)

      return ;

    string strkey = strtmp1.substr(first, last-first+1);

    first = (int)strtmp2.find_first_not_of(" \t");

    if(((last = (int)strtmp2.find("\t#", first )) != -1) ||

      ((last = (int)strtmp2.find(" #", first )) != -1) ||

      ((last = (int)strtmp2.find("\t\\", first )) != -1)||

      ((last = (int)strtmp2.find(" \\", first )) != -1))

    {

      strtmp2 = strtmp2.substr(0, last-first);

    }

    last = (int)strtmp2.find_last_not_of(" \t");

    if(first == string::npos || last == string::npos)

      return ;

    string value = strtmp2.substr(first, last-first+1);

    string mapkey = strsect + MIDDLESTRING;

    mapkey += strkey;

    (*pmap)[mapkey]=value;

    return ;

  }

};



class CHandle_IniFile

{

public:

  CHandle_IniFile( ){};

  ~CHandle_IniFile( )

  {

    c_inimap.clear();

    vt_ini.clear();

  };

  bool open(const char* pinipath)

  {

    return do_open(pinipath);

  }

  /* 

  *    读取对应的值

  *    description:

  *           psect = 主键 

  *           pkey  = 次级键

  *  return = value

  *

  */

  string read(const char*psect, const char*pkey)

  {

    string mapkey = psect;

    mapkey += MIDDLESTRING;

    mapkey += pkey;

    strMapIt it = c_inimap.find(mapkey);

    if(it == c_inimap.end())

      return "";

    else

      return it->second;

  }

  /* 

  *

  *  修改主键 (rootkey)

  *  description:

  *      sItself    = 要修改的主键名

  *           sNewItself = 目标主键名

  *   return = true(成功) or false(失败)

  *

  */ 

  bool change_rootkey(char *sItself,char *sNewItself)

  {

    if(!strcmp(sItself,"") || !strcmp(sNewItself,""))

    {

      return false;

    }

    string sIt = "[";

    sIt+=sItself;

    sIt+="]"; 

    for(std::vector<string>::iterator iter = vt_ini.begin();

      iter != vt_ini.end();iter++)

    {

      if(!(*iter).compare(sIt))

      {

        //** 改变文件vector

        sIt = "[";

        sIt+=sNewItself;

        sIt+="]"; 

        *iter = sIt;

        //** 改变map

        c_inimap.clear();

        for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

        return true;

      }

    }

    return false;

  }

  /* 

  *    修改次键 subkey 

  *    description:

  *      sRootkey    = 要修改的次键所属的主键名

  *           sItSubkey   = 要修改的次键名

  *           sNewSubkey  = 目标次键名

  *   return = true(成功) or false(失败)

  *

  */

  bool change_subkey(char *sRootkey,

    char *sItSubkey,

    char *sNewSubkey)

  {

    if(!strcmp(sRootkey,"") || !strcmp(sItSubkey,"") || !strcmp(sNewSubkey,""))

    {

      return false;

    }

    string sRoot = "[";

    sRoot+=sRootkey;

    sRoot+="]"; 

    int i = 0;

    for(std::vector<string>::iterator iter = vt_ini.begin();

      iter != vt_ini.end();iter++)

    {

      if(!(*iter).compare(sRoot))

      {

        //** 改变文件vector

        i=1;

        continue;

      }

      if(i)

      {

        if(0 == (*iter).find("["))

        {

          /* 没找到,已经到下一个 root */

          return false;

        }

        if(0 == (*iter).find(sItSubkey))

        {

          /* 已经找到 */

          string save = (*iter).substr(strlen(sItSubkey),(*iter).length());

          *iter  = sNewSubkey;

          *iter +=save;

          //** 改变map

          c_inimap.clear();

          for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

          return true;

        }

      }

    }

    return false;

  }

  /* 

  *    修改键值 

  *    description:

  *      sRootkey    = 要修改的次键所属的主键名

  *           sSubkey     = 要修改的次键名

  *           sValue      = 次键对应的值

  *   return = true(成功) or false(失败)

  *

  */

  bool change_keyvalue(char *sRootkey,

    char *sSubkey,

    char *sValue)

  {

    if(!strcmp(sRootkey,"") || !strcmp(sSubkey,""))

    {

      return false;

    }

    string sRoot = "[";

    sRoot+=sRootkey;

    sRoot+="]"; 

    int i = 0;

    for(std::vector<string>::iterator iter = vt_ini.begin();

      iter != vt_ini.end();iter++)

    {

      if(!(*iter).compare(sRoot))

      {

        //** 改变文件vector

        i=1;

        continue;

      }

      if(i)

      {

        if(0 == (*iter).find("["))

        {

          /* 没找到,已经到下一个 root */

          return false;

        }

        if(0 == (*iter).find(sSubkey))

        {

          /* 已经找到 */

          string save = (*iter).substr(0,strlen(sSubkey)+1);

          *iter  = save;

          *iter +=sValue;

          //** 改变map

          c_inimap.clear();

          for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

          return true;

        }

      }

    }

    return false;

  }

  /* 

  *    删除键值 

  *    description:

  *      sRootkey    = 要删除的主键名

  *           sSubkey     = 要删除的次键名 (如果为空就删除主键和主键下的所有次键)

  *    

  *   return = true(成功) or false(失败)

  *

  */

  bool del_key(char *sRootkey,

    char *sSubkey="")

  {
    return false;

    //if(!strcmp(sRootkey,""))

    //{

    //  return false;

    //}

    //string sRoot = "[";

    //sRoot+=sRootkey;

    //sRoot+="]"; 

    //if(!strcmp(sSubkey,""))

    //{

    //  //** del root key

    //  int last_boud = 0;

    //  int pad_size = 0;

    //  std::vector<string>::iterator at_begin,at_end = (std::vector<string>::iterator )0;

    //  for(std::vector<string>::iterator iter = vt_ini.begin();

    //    iter != vt_ini.end();iter++)

    //  {

    //    if(!(*iter).compare(sRoot))

    //    {

    //      //** 改变文件vector

    //      last_boud = 1;

    //      pad_size++;

    //      at_begin = iter;

    //      continue;

    //    }

    //    if(last_boud)

    //    {

    //      /* 已经到下一个 root */

    //      if(0 == (*iter).find("["))

    //      {

    //        at_end = iter;

    //        break;

    //      }

    //      pad_size++;

    //    }



    //  }

    //  if(!last_boud)

    //  {

    //    return false;/* 没找到*/

    //  }

    //  /* 替换 */

    //  if(at_end !=(std::vector<string>::iterator )0)

    //  {

    //    for(std::vector<string>::iterator pIter = at_begin;

    //      at_end != vt_ini.end();pIter++,at_end++)

    //    {

    //      *pIter = *at_end;

    //    }

    //  }

    //  while(pad_size)

    //  {

    //    vt_ini.pop_back();

    //    pad_size--;

    //  }

    //}else

    //  //** del sub key

    //{

    //  int last_boud = 0;

    //  for(std::vector<string>::iterator iter = vt_ini.begin();

    //    iter != vt_ini.end();iter++)

    //  {

    //    if(!(*iter).compare(sRoot))

    //    {

    //      //** 改变文件vector

    //      last_boud = 1;

    //      continue;

    //    }

    //    if(last_boud)

    //    {

    //      if(0 == (*iter).find("["))

    //      {

    //        /* 没找到,已经到下一个 root */

    //        return false;

    //      }

    //      if(0 == (*iter).find(sSubkey))

    //      {

    //        /* 已经找到 */

    //        if((iter+1) == vt_ini.end())

    //        {

    //          vt_ini.pop_back();

    //          break;

    //        }

    //        for(std::vector<string>::iterator it = iter+1;

    //          it != vt_ini.end();it++)

    //        {

    //          /* del one 向前移位*/

    //          *(it-1) = *it;

    //        }

    //        vt_ini.pop_back();

    //        break;

    //      }



    //    }

    //  }

    //  if(!last_boud)

    //    return false;

    //}

    ////** 改变map

    //c_inimap.clear();

    //for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

    //return true;

  }

  /* 

  *    增加键值

  *    description:

  *      1、sRootkey    = 要增加的主键名

  *           2、sSubkey     = 要增加的次键名 

  *           3、sKeyValue   = 要增加的次键值

  *

  *    (** 如果为1 2 同时为空就只增加主键，如果同时不为空就增加确定的键 **)

  *

  *   return = true(成功) or false(失败)

  *

  */

  bool add_key(char *sRootkey,

    char *sSubkey="",

    char *sKeyValue="")

  {

    if(!strcmp(sRootkey,""))

    {

      return false;

    }

    string sRoot = "[";

    sRoot+=sRootkey;

    sRoot+="]"; 

    bool isEnd = false;

    for(std::vector<string>::iterator iter = vt_ini.begin();

      iter != vt_ini.end();iter++)

    {

      if(!(*iter).compare(sRoot))

      {

        iter++;

        //** 改变文件vector

        if(!strcmp(sSubkey,""))

        {

          return true;

        }else

        {

          if(!strcmp(sKeyValue,""))

          {

            return false;

          }

          //** 继续找次键

          while(iter != vt_ini.end())

          {

            if(0 == (*iter).find("["))

            {

              /* 没找到,已经到下一个 root 

              添加

              */

              string push_node = *(vt_ini.end()-1);

              std::vector<string>::iterator p_back ;

              for(p_back = vt_ini.end()-1;

                p_back != iter;p_back--)

              {

                *p_back  = *(p_back-1);

              }

              *p_back = sSubkey;

              *p_back += "=";

              *p_back += sKeyValue;

              vt_ini.push_back(push_node);

              //** 改变map

              c_inimap.clear();

              for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

              return true;



            }else

            {

              /*

              找到了替换

              */

              if(0 == (*iter).find(sSubkey))

              {

                string save = (*iter).substr(0,strlen(sSubkey)+1);

                *iter  = save;

                *iter +=sKeyValue;



                //** 改变map

                c_inimap.clear();

                for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

                return true;

              }

            }

            iter++;

          }

          /* 没找到 */      

          isEnd = true;

          break;

        }

      }

    }

    /* 没找到增加 */

    if(strcmp(sSubkey,"") && strcmp(sKeyValue,""))

    {

      if(!isEnd)

      {

        vt_ini.push_back(sRoot);

      }

      string st = sSubkey;

      st += "=";

      st += sKeyValue;

      vt_ini.push_back(st);



    }else if(!strcmp(sSubkey,"") && !strcmp(sKeyValue,""))

    {     

      vt_ini.push_back(sRoot);

    }else

    {

      return false;

    }

    //** 改变map

    c_inimap.clear();

    for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

    return true;

  }

  /* 

  *    save ini file 

  *

  *    提供缓冲机制，当涉及到修改、删除、增加操作时，

  *    不会立即写如文件，只是改写内存，当调用次函数

  *    时写到文件，保存

  *

  */

  bool flush()

  {

    ofstream out(s_file_path.c_str());

    if(!out.is_open())

      return false;

    copy(vt_ini.begin(),vt_ini.end()-1,ostream_iterator<string>(out,"/n"));

    copy(vt_ini.end()-1,vt_ini.end(),ostream_iterator<string>(out,""));

    out.close();

    return true;

  }

protected:

  /* 打开 ini 文件*/

  bool do_open(const char* pinipath)

  {

    s_file_path = pinipath;

    ifstream fin(pinipath);

    if(!fin.is_open())

      return false;

    while(!fin.eof())

    {

      string inbuf;
      getline(fin, inbuf,'\n');
      if (inbuf.length() > 0 && inbuf[inbuf.length() - 1] == '\r')
      {
        inbuf = inbuf.substr(0, inbuf.length() - 1);
      }
      //printf("len=%d,[%s]\n", inbuf.length(), inbuf.c_str());

      if(inbuf.empty())

        continue;

      vt_ini.push_back(inbuf);

    }

    fin.close();

    if(vt_ini.empty())

      return true;

    for_each(vt_ini.begin(), vt_ini.end(), analyzeini(c_inimap));

    return true;        

  }

private:

  /* file path */

  string s_file_path;

  /* 保存键值对应  key = value */

  strMap    c_inimap;

  /* 保存整个文件信息 */

  vector<string> vt_ini;

};


