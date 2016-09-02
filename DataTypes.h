#ifndef _DATATYPES_H_INCLUDED
#define _DATATYPES_H_INCLUDED

#define NTOHS
#define NTOHL
#define HTONS
#define HTONL

#ifdef _WIN32
#include <winsock2.h>
typedef int	socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#endif

#ifndef uint8
  typedef unsigned char uint8;
#endif

#ifndef uint16
  typedef unsigned short uint16;
#endif
  
#ifndef uint32
  typedef unsigned int uint32;
#endif

#pragma pack(1)
typedef struct tagPacketHead
{
  uint16  sync_tag;
  uint8   ver;
  uint16  packet_length;
  uint8   payload_id;
  bool Verify(){return NTOHS(sync_tag) == 0xAA55 && ver == 0x01 && NTOHS(packet_length) >= sizeof(tagPacketHead);}
  int GetBodyLen(){return NTOHS(packet_length) - sizeof(tagPacketHead);}
}PacketHead;
#pragma pack()

enum ErrorCode
{
  ERROR_OK = 0x00,
  ERROR_DROP,

  ERROR_PACKET = 0x10,
  ERROR_PACKET_CRC,         /// CRC校验失败
  ERROR_PACKET_LEN,         /// 报文长度不符
  ERROR_PACKET_PAYLOAD,     /// 报名命令字错误
  ERROR_PACKET_DATA,        /// 数据格式有误
  ERROR_PACKET_VERIFY,      /// 验证签名失败
  ERROR_PACKET_TIMEOUT,     /// 超时未处理数据
  ERROR_PACKET_DUPPAYLOAD,  /// 重复请求同一个业务

  ERROR_DATABASE = 0x20,
  ERROR_DATABASE_NOCONN,    /// 无可用数据库连接
  ERROR_DATABASE_EXECUTE    /// 数据库执行错误
};

#endif
