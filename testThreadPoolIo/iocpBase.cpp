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

int postSend(PMyCompleteKey pkCK, myBuffer kSend)
{
	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_WSASEND;
	pkOL->kBuffer = std::move(kSend);
	pkOL->pkCk = pkCK;
	pkOL->kWSABuf.buf = pkOL->kBuffer.data();
	pkOL->kWSABuf.len = pkOL->kBuffer.size();
	unsigned long	ulTransmitBytes = 0;

	StartThreadpoolIo(pkCK->pkIO);

	int iError = ERROR_SUCCESS;
	if (::WSASend(
		pkCK->uiSocket,
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
			kSend = std::move(pkOL->kBuffer);
			CancelThreadpoolIo(pkCK->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

int postAccept(LPFN_ACCEPTEX pfnAcceptEx, PMyCompleteKey pkCKListen, PMyCompleteKey pkCK)
{
	int iError = ERROR_SUCCESS;

	DWORD ulRecvBytes = 0;
	DWORD ulReceiveDataBytes = 0;
	DWORD ulLocalAddressBytes = sizeof(SOCKADDR_IN) + 16;
	DWORD ulRemoteAddressBytes = sizeof(SOCKADDR_IN) + 16;

	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_ACCEPTEX;
	pkOL->kWSABuf.buf = pkOL->kBuffer.data();
	pkOL->kWSABuf.len = pkOL->kBuffer.size();
	pkOL->pkCk = pkCK;

	StartThreadpoolIo(pkCKListen->pkIO);

	if (pfnAcceptEx(
		pkCKListen->uiSocket,
		pkCK->uiSocket,
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
			CancelThreadpoolIo(pkCK->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

int postRecv(PMyCompleteKey pkCK)
{
	int iError = ERROR_SUCCESS;
	DWORD	ulFlag = 0;
	DWORD	ulTransmitBytes = 0;

	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_WSARECV;
	pkOL->kWSABuf.buf = pkOL->kBuffer.data();
	pkOL->kWSABuf.len = pkOL->kBuffer.writableBytes();
	pkOL->pkCk = pkCK;

	StartThreadpoolIo(pkCK->pkIO);

	if (::WSARecv(
		pkCK->uiSocket,
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
			CancelThreadpoolIo(pkCK->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

int postDisconnect(LPFN_DISCONNECTEX pfnDisconnectEx, PMyCompleteKey pkCK)
{
	int iError = ERROR_SUCCESS;

	PMyOverlapped pkOL = new MyOverlapped();
	pkOL->eType = E_OLT_WSADISCONNECTEX;
	pkOL->kWSABuf.buf = pkOL->kBuffer.data();
	pkOL->kWSABuf.len = pkOL->kBuffer.size();
	pkOL->pkCk = pkCK;

	StartThreadpoolIo(pkCK->pkIO);

	if (pfnDisconnectEx(
		pkCK->uiSocket,
		&pkOL->kOverlapped,
		TF_REUSE_SOCKET,
		0) == SOCKET_ERROR)
	{
		iError = ::WSAGetLastError();
		if (iError != WSA_IO_PENDING)
		{
			CancelThreadpoolIo(pkCK->pkIO);
			delete pkOL;
		}
	}

	return iError;
}

void getSocketAddrs(LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs,
	PMyCompleteKey pkCK,
	char* pAcceptInfo,
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
	
	InetNtop(pkLocalAddr->sin_family, (PVOID)&pkLocalAddr->sin_addr, pkCK->pcLocalIp, 16);
	pkCK->usLocalPort = htons(pkLocalAddr->sin_port);

	InetNtop(pkRemoteAddr->sin_family, (PVOID)&pkRemoteAddr->sin_addr, pkCK->pcRemoteIp, 16);
	pkCK->usRemotePort = htons(pkRemoteAddr->sin_port);
}














