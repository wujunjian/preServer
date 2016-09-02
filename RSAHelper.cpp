#include "RSAHelper.h"
#include "Utils.h"
#include <openssl/rsa.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/err.h>

RSAHelper::RSAHelper(void)
{
}


RSAHelper::~RSAHelper(void)
{
}

int RSAHelper::MD5ToBin(const char *src, int len, unsigned char *outbuf)
{
  memset(outbuf, 0, 16);
  MD5_CTX ctx;
  MD5_Init(&ctx);
  MD5_Update(&ctx, src, len);
  MD5_Final(outbuf, &ctx);
  return 0;
}

string RSAHelper::MD5ToStr(const char *src, int len)
{
  unsigned char outbuf[16];
  MD5ToBin(src, len, outbuf);
  char str[33];
  Utils::BinToHexStr((const char *)outbuf, sizeof(outbuf), str);
  str[sizeof(str) - 1] = '\0';
  return str;
}

int RSAHelper::Verify(const char *src, int srclen, char *encrypted, const char *n)
{
  // 利用公钥解密
  BIGNUM *bnn, *bne, *bnd;

  bnn = BN_new();
  bne = BN_new();
  bnd = BN_new();
  BN_hex2bn(&bnn, n);
  BN_set_word(bne, RSA_F4);

  RSA *r = RSA_new();
  r->n = bnn;
  r->e = bne;
  r->d = bnd;
  int flen = RSA_size(r);// 

  unsigned char outdata[256];
  int ret = RSA_public_decrypt(flen, (const unsigned char *)encrypted, outdata, r, RSA_PKCS1_PADDING);
  RSA_free(r);

  // 源数据加密
  char str[16];
  
  MD5ToBin(src, srclen, (unsigned char *)str);
  
  // 数据对比
  return memcmp(str, outdata, sizeof(str));
}
