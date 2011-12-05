/**********************************************************************
  TEMPORARY NET STUFF 

  rli
  ********************************************************************/

#ifndef _RLI_NET_
#define _RLI_NET_

#include <winsock2.h>
#include <ws2tcpip.h>

/////////////////////////////////////////////////////////////////////
// PatchServer stuff
#define FILE_CHECK				((UINT8)1)
#define FILE_EXIST				((UINT8)2)
#define FILE_TRANSFER_YES		((UINT8)1)
#define FILE_TRANSFER_NO		((UINT8)2)

#define PATCH_SERVER_IP			"127.0.0.1"
#define PATCH_SERVER_PORT		"12101"

//////////////////////////////////////////////////////////////////////

#define RET_IF(exp, ret) {if (exp) return (ret);}
#define RET_IF_NOT(exp, ret) RET_IF(!(exp), ret)
#define RET_FALSE_IF_NOT(exp) RET_IF_NOT(exp, FALSE)

SOCKET ClientConnectTCP(LPCTSTR server_ip, LPCTSTR server_port);

BOOL ClientSendTCP(SOCKET sock, const UINT8* buf, UINT32 len, UINT32 flags = 0);
BOOL ClientRecvTCP(SOCKET sock, UINT8* buf, UINT32 len, UINT32 flags = 0);

SOCKET ServerListenTCP(LPCTSTR server_port);

BOOL ServerSendTCP(SOCKET sock, const UINT8* buf, UINT32 len, UINT32 flags = 0);
BOOL ServerRecvTCP(SOCKET sock, UINT8* buf, UINT32 len, UINT32 flags = 0);






#endif // _RLI_NET_