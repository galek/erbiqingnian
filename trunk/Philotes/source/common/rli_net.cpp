
#include "rli_net.h"

#define DEFAULT_BACKLOG 10

SOCKET ClientConnectTCP(LPCTSTR server_ip, LPCTSTR server_port)
{
	SOCKET sockConnect;
	struct addrinfo* result;
	struct addrinfo* ptr;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(server_ip, server_port, &hints, &result) != 0) {
		return (SOCKET)SOCKET_ERROR;
	}
	sockConnect = (SOCKET)SOCKET_ERROR;
	for (ptr = result; ptr; ptr = ptr->ai_next) {
		sockConnect = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sockConnect == INVALID_SOCKET) {
			freeaddrinfo(result);
			return (SOCKET)SOCKET_ERROR;
		}

		if (connect(sockConnect, ptr->ai_addr, (int)(ptr->ai_addrlen)) != SOCKET_ERROR) {
			return sockConnect;
		} else {
			closesocket(sockConnect);
		}
	}
	return (SOCKET)SOCKET_ERROR;
}

BOOL ClientSendTCP(SOCKET sock, const UINT8* buffer, UINT32 len, UINT32 flags)
{
	ASSERT_RETTRUE(len > 0);

	UINT32 sent = 0;
	INT32 result;

	while (sent < len) {
		result = send(sock, (const char*)(buffer + sent * sizeof(UINT8)), len - sent, flags);
		if (result < 0) {
			return FALSE;
		} else {
			sent += (UINT32)result;
		}
	}
	return TRUE;
}

BOOL ClientRecvTCP(SOCKET sock, UINT8* buf, UINT32 len, UINT32 flags)
{
	ASSERT_RETTRUE(len > 0);
	UINT32 total = 0, cur;

	while (total < len) {
		cur = recv(sock, (char*)(buf + total * sizeof(UINT8)), len - total, flags);
		if (cur > 0) {
			total += cur;
		} else {
			break;
		}
	}
	if (total < len) {
		return FALSE;
	} else {
		return TRUE;
	}
}

SOCKET ServerListenTCP(LPCTSTR server_port)
{
	SOCKET sockListen;
	struct addrinfo* result;
	struct addrinfo hints;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
		return (SOCKET)SOCKET_ERROR;
	}

	sockListen = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (sockListen == INVALID_SOCKET) {
		freeaddrinfo(result);
		return (SOCKET)SOCKET_ERROR;
	}

	if (bind(sockListen, result->ai_addr, (int)(result->ai_addrlen)) != 0) {
		freeaddrinfo(result);
		closesocket(sockListen);
		return (SOCKET)SOCKET_ERROR;
	}

	if (listen(sockListen, DEFAULT_BACKLOG) == SOCKET_ERROR) {
		freeaddrinfo(result);
		closesocket(sockListen);
		return (SOCKET)SOCKET_ERROR;
	}
	
	freeaddrinfo(result);
	return sockListen;
}

BOOL ServerSendTCP(SOCKET sock, const UINT8* buf, UINT32 len, UINT32 flags)
{
	return ClientSendTCP(sock, buf, len, flags);
}

BOOL ServerRecvTCP(SOCKET sock, UINT8* buf, UINT32 len, UINT32 flags)
{
	return ClientRecvTCP(sock, buf, len, flags);
}
