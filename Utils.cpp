#include "Utils.h"
#include <stdio.h>
#include <string>

string Utils::ConvertDT(const char *strDateTime)
{
  return "";
}

char Utils::HexToBin(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  return 0;
}

char Utils::HexToBin(char ch, char cl)
{
  return (HexToBin(ch) << 4) | HexToBin(cl);
}

int Utils::HexStrToBin(const char *hexstr, int hexlen, char *bin)
{
  for (int i = 0; i < hexlen / 2; i++)
  {
    bin[i] = HexToBin(hexstr[i * 2], hexstr[i * 2 + 1]);
  }
  
  return 0;
}

int Utils::BinToHexStr(const char *bin, int binlen, char *hexstr)
{
  for (int i = 0; i < binlen; i++)
  {
    sprintf(hexstr + 2 * i, "%02x", *((unsigned char *)bin + i));
  }
  hexstr[binlen * 2] = '\0';
  
  return 0;
}

/************************************************
方法名:CheckUcharEnd
描述: 检查结构体中 uchar数组的结尾是否为 \0
作者: phl
日期:

修改:
	修改人:
	修改时间:

************************************************/
bool 
Utils::CheckUcharEnd(char* pIN, int index)
{
	if(pIN[index-1] == '\0')
	{
		return true;
	}
	else
	{
		if(strlen(pIN) < index)
		{
			return true;
		}
		return false;
	}

}
