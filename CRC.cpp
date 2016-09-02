#include "CRC.h"

unsigned short CalcCrc16(unsigned char * pData, int nLength)
{
  unsigned short cRc_16 = 0x0000;
  const unsigned short cnCRC_16 = 0x8005;
  unsigned long cRctable_16[256];
  unsigned short i,j,k;
  for (i=0,k=0;i<256;i++,k++)
  {
    cRc_16 = i<<8;
    for (j=8;j>0;j--)
    {
      if (cRc_16&0x8000)
        cRc_16 = (cRc_16<<=1)^cnCRC_16;
      else
        cRc_16<<=1;
    }
    cRctable_16[k] = cRc_16;
  }
  while (nLength>0)
  {
    cRc_16 = (cRc_16 << 8) ^ cRctable_16[((cRc_16>>8) ^ *pData) & 0xff];
    nLength--;
    pData++;
  }
  return cRc_16;
}
