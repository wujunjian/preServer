#pragma once
#include <string>
#include <cstring>
using namespace std;

class RSAHelper
{
public:
  RSAHelper(void);
  virtual ~RSAHelper(void);
  static int MD5ToBin(const char *src, int len, unsigned char *outbuf);
  static int MD5ToBin(const char *src, unsigned char *outbuf){return MD5ToBin(src, strlen(src), outbuf);}
  static string MD5ToStr(const char *src, int len);
  static string MD5ToStr(const char *src){return MD5ToStr(src, strlen(src));}
  static int Verify(const char *src, int srclen, char *encrypted, const char *n);
};

