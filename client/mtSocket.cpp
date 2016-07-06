#include <Winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <Ws2tcpip.h>

SOCKET mtClientSocket(const char* pcIP, unsigned short usPort)
{
	hostent*	phostent = NULL;

	SOCKET	uiSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	DWORD	ulErrno = WSAGetLastError();
	if (INVALID_SOCKET == uiSocket) {
		printf("error: create socket=[%d] failed, ip=[%s], port=[%d], errno=[%d], thread=[%d]\n",
			uiSocket, pcIP, usPort, ulErrno, GetCurrentThreadId());
		return	SOCKET_ERROR;
	}

	SOCKADDR_IN			kSockAddrIn;
	kSockAddrIn.sin_family = AF_INET;
	
	InetPton(kSockAddrIn.sin_family, pcIP, &kSockAddrIn.sin_addr);

	kSockAddrIn.sin_port = htons(usPort);
	int		iConnectRet = connect(uiSocket, (SOCKADDR*)&kSockAddrIn, sizeof(SOCKADDR_IN));

	if (0 == iConnectRet) {
		printf("connect server succeed, socket=[%d], ip=[%s], port=[%d], succeed, thread=[%d]\n",
			uiSocket, pcIP, usPort, GetCurrentThreadId());
	}
	else
	{
		ulErrno = WSAGetLastError();
		printf("error: connect server failed, socket=[%d] failed, ip=[%s], port=[%d], errno=[%d], thread=[%d]\n",
			uiSocket, pcIP, usPort, ulErrno, GetCurrentThreadId());
		return	SOCKET_ERROR;
	}

	bool bNoDelay = true;
	if (setsockopt(uiSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&bNoDelay, sizeof(bNoDelay)))
	{
		ulErrno = WSAGetLastError();
		printf("error: setsockopt TCP_NODELAY socket=[%d] failed, ip=[%s], port=[%d], errno=[%d], thread=[%d]\n",
			uiSocket, pcIP, usPort, ulErrno, GetCurrentThreadId());
		return	SOCKET_ERROR;
	}

	return	uiSocket;
}

int mtClientRecv(SOCKET uiSocket, char* pcData, int iBytes)
{
	int		iRet = 0;

	int		iNeed = iBytes;
	int		iReal = 0;

	do {
		iRet = recv(uiSocket, pcData + iReal, iNeed, 0);
		if (iRet < 0)
		{
			printf("warning: recv data failed, need=[%d] real=[%d] errno=[%d] thread=[%d]\n",
				iBytes, iRet, WSAGetLastError(), GetCurrentThreadId());
			return	-1;
		}
		else if (iRet == 0)
		{
			printf("warning: server active disconnect, recv data failed, need=[%d] real=[%d] errno=[%d] thread=[%d]\n",
				iBytes, iReal, WSAGetLastError(), GetCurrentThreadId());
			return	-2;
		}
		else
		{
			iReal += iRet;
			iNeed -= iRet;
		}
	} while (iReal != iBytes);

	return	1;
}

int mtClientSend(SOCKET uiSocket, char* pcData, int iBytes)
{
	int		iRet = send(uiSocket, pcData, iBytes, 0);

	if (iBytes != iRet) {
		DWORD	ulErrno = WSAGetLastError();
		printf("warning: send data failed, need=[%d] real=[%d] errno=[%d] thread=[%d]\n",
			iBytes, iRet, ulErrno, GetCurrentThreadId());
		return	0;
	}

	return	1;
}