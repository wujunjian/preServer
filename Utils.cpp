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
������:CheckUcharEnd
����: ���ṹ���� uchar����Ľ�β�Ƿ�Ϊ \0
����: phl
����:

�޸�:
	�޸���:
	�޸�ʱ��:

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
