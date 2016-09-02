#pragma once
#include <string>
#include <string.h>
#include <iostream>
using namespace std;

class Utils
{
public:
  static string ConvertDT(const char *strDateTime);
  static int HexStrToBin(const char *hexstr, int hexlen, char *bin);
  static char HexToBin(char c);
  static char HexToBin(char ch, char cl);
  
  
  static bool CheckUcharEnd(char* pIN, int index);
  static int BinToHexStr(const char *bin, int binlen, char *hexstr);
};
