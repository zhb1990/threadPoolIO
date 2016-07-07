#include "iocpBase.h"
#include <Ws2tcpip.h>

volatile long lCntBufferCreate = 0;
volatile long lCntBufferRelease = 0;

volatile long lCnt_CompleteKeyCreate = 0;
volatile long lCnt_CompleteKeyRelease = 0;

volatile long lCnt_OverlappedCreate = 0;
volatile long lCnt_OverlappedRelease = 0;

int getExtensionFuncPtr(LPVOID* plpf, SOCKET sock, GUID guid)
{
	DWORD dwBytes;
	*plpf = NULL;

	int iError = ERROR_SUCCESS;
	if (::WSAIoctl(sock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&guid,
		sizeof(guid),
		plpf,
		sizeof(*plpf),
		&dwBytes,
		NULL,
		NULL) == SOCKET_ERROR) {
		iError = WSAGetLastError();
	}

	return iError;
}

int postSend(MySharedCompleteKey kSharedCk, myBuffer& rSend)
{
	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_WSASEND;
	pkOL->kBuffer = std::move(rSend);
	pkOL->kSharedCk = kSharedCk;
	pkOL->kWSABuf.buf = pkOL->kBuffer.beginRead();
	pkOL->kWSABuf.len = pkOL->kBuffer.size();
	unsigned long	ulTransmitBytes = 0;

	StartThreadpoolIo(kSharedCk->pkIO);

	int iError = ERROR_SUCCESS;
	if (::WSASend(
		kSharedCk->uiSocket,
		&pkOL->kWSABuf,
		1,
		&ulTransmitBytes,
		0,
		&pkOL->kOverlapped,
		NULL) == SOCKET_ERROR)
	{
		iError = ::WSAGetLastError();
		if (iError != WSA_IO_PENDING)
		{
			rSend = std::move(pkOL->kBuffer);
			CancelThreadpoolIo(kSharedCk->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

int postAccept(LPFN_ACCEPTEX pfnAcceptEx, 
	MySharedCompleteKey kSharedCkListen, 
	MySharedCompleteKey kSharedCk)
{
	int iError = ERROR_SUCCESS;

	DWORD ulRecvBytes = 0;
	DWORD ulReceiveDataBytes = 0;
	DWORD ulLocalAddressBytes = sizeof(SOCKADDR_IN) + 16;
	DWORD ulRemoteAddressBytes = sizeof(SOCKADDR_IN) + 16;

	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_ACCEPTEX;
	pkOL->kWSABuf.buf = pkOL->kBuffer.beginRead();
	pkOL->kWSABuf.len = pkOL->kBuffer.size();
	pkOL->kSharedCk = kSharedCk;

	StartThreadpoolIo(kSharedCkListen->pkIO);

	if (pfnAcceptEx(
		kSharedCkListen->uiSocket,
		kSharedCk->uiSocket,
		pkOL->kWSABuf.buf,
		ulReceiveDataBytes,
		ulLocalAddressBytes,
		ulRemoteAddressBytes,
		&ulRecvBytes,
		&pkOL->kOverlapped) == SOCKET_ERROR)
	{
		iError = ::WSAGetLastError();
		if (iError != WSA_IO_PENDING)
		{
			CancelThreadpoolIo(kSharedCk->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

int postRecv(MySharedCompleteKey kSharedCk)
{
	int iError = ERROR_SUCCESS;
	DWORD	ulFlag = 0;
	DWORD	ulTransmitBytes = 0;

	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_WSARECV;
	pkOL->kWSABuf.buf = pkOL->kBuffer.beginRead();
	pkOL->kWSABuf.len = pkOL->kBuffer.writableBytes();
	pkOL->kSharedCk = kSharedCk;

	StartThreadpoolIo(kSharedCk->pkIO);

	if (::WSARecv(
		kSharedCk->uiSocket,
		&pkOL->kWSABuf,
		1,
		&ulTransmitBytes,
		&ulFlag,
		&pkOL->kOverlapped,
		NULL) == SOCKET_ERROR)
	{
		iError = ::WSAGetLastError();
		if (iError != WSA_IO_PENDING)
		{
			CancelThreadpoolIo(kSharedCk->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

int postDisconnect(LPFN_DISCONNECTEX pfnDisconnectEx, MySharedCompleteKey kSharedCk)
{
	int iError = ERROR_SUCCESS;

	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_WSADISCONNECTEX;
	pkOL->kWSABuf.buf = pkOL->kBuffer.beginRead();
	pkOL->kWSABuf.len = pkOL->kBuffer.size();
	pkOL->kSharedCk = kSharedCk;

	StartThreadpoolIo(kSharedCk->pkIO);

	if (pfnDisconnectEx(
		kSharedCk->uiSocket,
		&pkOL->kOverlapped,
		TF_REUSE_SOCKET,
		0) == SOCKET_ERROR)
	{
		iError = ::WSAGetLastError();
		if (iError != WSA_IO_PENDING)
		{
			CancelThreadpoolIo(kSharedCk->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

void getSocketAddrs(LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs, 
	MySharedCompleteKey kSharedCk, 
	char * pAcceptInfo, 
	int iRecvBytes)
{
	sockaddr_in*	pkLocalAddr = NULL;
	int				iLocalBytes = sizeof(sockaddr_in);
	sockaddr_in*	pkRemoteAddr = NULL;
	int				iRemoteBytes = sizeof(sockaddr_in);

	unsigned long ulReceiveDataBytes = 0;
	unsigned long ulLocalAddressBytes = sizeof(SOCKADDR_IN) + 16;
	unsigned long ulRemoteAddressBytes = sizeof(SOCKADDR_IN) + 16;

	lpfnGetAcceptExSockAddrs(pAcceptInfo, iRecvBytes, ulLocalAddressBytes, ulRemoteAddressBytes,
		(sockaddr**)&pkLocalAddr, &iLocalBytes, (sockaddr**)&pkRemoteAddr, &iRemoteBytes);
	
	InetNtop(pkLocalAddr->sin_family, (PVOID)&pkLocalAddr->sin_addr, kSharedCk->pcLocalIp, 16);
	kSharedCk->usLocalPort = htons(pkLocalAddr->sin_port);

	InetNtop(pkRemoteAddr->sin_family, (PVOID)&pkRemoteAddr->sin_addr, kSharedCk->pcRemoteIp, 16);
	kSharedCk->usRemotePort = htons(pkRemoteAddr->sin_port);
}














