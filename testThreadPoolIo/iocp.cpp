#include <stdio.h>
#include "iocp.h"

VOID CALLBACK IoCompletionCallback(PTP_CALLBACK_INSTANCE Instance, 
	PVOID Context, 
	PVOID Overlapped, 
	ULONG IoResult, 
	ULONG_PTR NumberOfBytesTransferred, 
	PTP_IO Io)
{
	PMyOverlapped pkOL = NULL;
	if (Overlapped != NULL)
	{
		pkOL = CONTAINING_RECORD(Overlapped, MyOverlapped, kOverlapped);
	}
	MyIOCP* pkIOCP = (MyIOCP*)Context;

	do 
	{
		if (pkOL == NULL || Io == NULL)
		{
			break;
		}
		MySharedCompleteKey kCK = pkOL->kSharedCk;
		if (kCK.get() == NULL)
		{
			delete pkOL;
			break;
		}
		switch (pkOL->eType)
		{
		case E_OLT_ACCEPTEX:
			pkIOCP->handAccept(IoResult, pkOL, NumberOfBytesTransferred);
			break;
		case E_OLT_WSADISCONNECTEX:
			break;
		case E_OLT_WSARECV:
			pkIOCP->handRecv(IoResult, pkOL, NumberOfBytesTransferred);
			break;
		case E_OLT_WSASEND:
			pkIOCP->handSend(IoResult, pkOL, NumberOfBytesTransferred);
			break;
		case E_OLT_CONNECTEX:
			break;
		default:
			break;
		}
	} while (0);
}

MyIOCP::MyIOCP(unsigned short usListenPort)
	:_usListenPort(usListenPort),
	_ListenCK(new MyCompleteKey())
{
	initExtensionFuncPtr();
	int iError = ERROR_SUCCESS;
	SOCKET uiListenSocket = createListenSocket(_usListenPort, iError);
	if (uiListenSocket == SOCKET_ERROR)
	{
		exit(0);
	}
	
	_ListenCK->uiSocket = uiListenSocket;
	_ListenCK->pkIO = CreateThreadpoolIo((HANDLE)_ListenCK->uiSocket,
		(PTP_WIN32_IO_CALLBACK)IoCompletionCallback,
		this, 
		NULL);
}

void MyIOCP::startAccept()
{
	for (int i = 0; i < 10; i ++)
	{
		acceptOne();
	}
}

int MyIOCP::acceptOne()
{
	int iError = ERROR_SUCCESS;
	SOCKET uiSocket = createAcceptSocket(iError);
	do
	{
		if (uiSocket == SOCKET_ERROR)
		{
			break;
		}
		MySharedCompleteKey kCK(new MyCompleteKey());
		if (kCK.get() == NULL)
		{
			closesocket(uiSocket);
			break;
		}
		kCK->uiSocket = uiSocket;
		kCK->pkIO = CreateThreadpoolIo((HANDLE)kCK->uiSocket,
			(PTP_WIN32_IO_CALLBACK)IoCompletionCallback,
			this,
			NULL);
		if (kCK->pkIO == NULL)
		{
			closesocket(uiSocket);
			break;
		}
		iError = reqAccept(kCK);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			closesocket(uiSocket);
			CloseThreadpoolIo(kCK->pkIO);
		}
	} while (0);

	return iError;
}

SOCKET MyIOCP::createListenSocket(unsigned short usListenPort, int& iError)
{
	iError = ERROR_SUCCESS;
	SOCKET uiListenSocket = SOCKET_ERROR;
	do 
	{
		/*create socket*/
		uiListenSocket = WSASocketW(
			AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP,
			NULL,
			0,
			WSA_FLAG_OVERLAPPED);
		if (SOCKET_ERROR == uiListenSocket)
		{
			iError = WSAGetLastError();
			break;
		}

		BOOL bReUseAddr = TRUE;
		if (setsockopt(uiListenSocket, 
			SOL_SOCKET,
			SO_REUSEADDR,
			(char*)&bReUseAddr,
			sizeof(bReUseAddr)) == SOCKET_ERROR)
		{
			iError = WSAGetLastError();
			closesocket(uiListenSocket);
			uiListenSocket = SOCKET_ERROR;
			break;
		}
		/*bind socket*/
		SOCKADDR_IN kSockAddrIn;
		memset(&kSockAddrIn, 0, sizeof(kSockAddrIn));

		kSockAddrIn.sin_family = AF_INET;
		kSockAddrIn.sin_addr.s_addr = ADDR_ANY;
		kSockAddrIn.sin_port = htons(usListenPort);

		if (bind(uiListenSocket,
			(SOCKADDR*)&kSockAddrIn,
			sizeof(kSockAddrIn)) == SOCKET_ERROR)
		{
			iError = WSAGetLastError();
			closesocket(uiListenSocket);
			uiListenSocket = SOCKET_ERROR;
			break;
		}

		/*listen socket*/
		if (listen(uiListenSocket, SOMAXCONN) == SOCKET_ERROR) 
		{
			iError = WSAGetLastError();
			closesocket(uiListenSocket);
			uiListenSocket = SOCKET_ERROR;
			break;
		}
	} while (0);

	return	uiListenSocket;
}

SOCKET MyIOCP::createAcceptSocket(int& iError)
{
	iError = ERROR_SUCCESS;
	SOCKET uiListenSocket = SOCKET_ERROR;
	do
	{
		/*create socket*/
		uiListenSocket = WSASocketW(
			AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP,
			NULL,
			0,
			WSA_FLAG_OVERLAPPED);
		if (SOCKET_ERROR == uiListenSocket)
		{
			iError = WSAGetLastError();
			break;
		}
	} while (0);

	return	uiListenSocket;
}

int MyIOCP::mySend(MySharedCompleteKey kCK, myBuffer& rSend)
{
	int iRet = FALSE;
	do
	{
		if (rSend.size() <= 0)
		{
			break;
		}
		int iError = reqSend(kCK, rSend);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			printf("SEND: FAIL [%s] -> ip[%s], port[%u], reqSend error=%d\n", rSend.beginRead(),
				kCK->pcRemoteIp, kCK->usRemotePort, iError);

			closesocket(kCK->uiSocket);

			break;
		}
		iRet = TRUE;
	} while (0);

	return iRet;
}
int MyIOCP::mySend(MySharedCompleteKey kCK, char* pcSend, int iLen)
{
	int iRet = FALSE;
	do 
	{
		if (iLen <= 0)
		{
			break;
		}
		iRet = mySend(kCK, myBuffer(pcSend, iLen));
	} while (0);
	
	return iRet;
}


void MyIOCP::handSend(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred)
{
	MySharedCompleteKey kCK = pkOL->kSharedCk;
	bool bSucc = false;
	do 
	{
		if (IoResult != ERROR_SUCCESS)
		{
			printf("SEND: FAIL [%s] -> ip[%s], port[%u], IoResult=%d\n", pkOL->kBuffer.beginRead(),
				kCK->pcRemoteIp, kCK->usRemotePort, IoResult);
			break;
		}

		printf("SEND:[%s] -> ip[%s], port[%u]\n", pkOL->kBuffer.beginRead(),
			kCK->pcRemoteIp, kCK->usRemotePort);

		myBuffer kBuffer;
		{
			CLocalLock<CCriSec400> kLocker(kCK->ccsSend);
			kCK->iSendCnt--;
			if (kCK->kSendList.size() > 0)
			{
				kBuffer = std::move(kCK->kSendList.front());
				kCK->kSendList.pop_front();
			}
		}
		if (kBuffer.size() > 0)
		{
			int iError = postSend(kCK, kBuffer);
			if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
			{
				printf("SEND: FAIL [%s] -> ip[%s], port[%u], postSend error=%d\n", kBuffer.beginRead(),
					kCK->pcRemoteIp, kCK->usRemotePort, iError);
				break;
			}
		}
		bSucc = true;
	} while (0);
	
	delete pkOL;
	if (!bSucc)
	{
		closesocket(kCK->uiSocket);
	}
}

void MyIOCP::handRecv(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred)
{
	MySharedCompleteKey kCK = pkOL->kSharedCk;
	bool bSucc = false;

	do
	{
		if (NumberOfBytesTransferred == 0)
		{
			printf("Active disconnect: ip[%s], port[%u]\n",
				kCK->pcRemoteIp, kCK->usRemotePort);
			break;
		}

		if (IoResult != ERROR_SUCCESS)
		{
			printf("RECV: FAIL ip[%s], port[%u], IoResult=%d\n",
				kCK->pcRemoteIp, kCK->usRemotePort, IoResult);
			break;
		}

		pkOL->kBuffer.hasWritten((int)NumberOfBytesTransferred);
		printf("RECV:[%s] <- ip[%s], port[%u]\n", pkOL->kBuffer.beginRead(),
			kCK->pcRemoteIp, kCK->usRemotePort);

		mySend(kCK, pkOL->kBuffer);

		int iError = reqRecv(kCK);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			printf("handRecv reqRecv fail ip[%s], port[%u], iError=%d\n", 
				kCK->pcRemoteIp, kCK->usRemotePort, iError);
			break;
		}
		
		bSucc = true;
	} while (0);

	delete pkOL;
	if (!bSucc)
	{
		closesocket(kCK->uiSocket);
		CloseThreadpoolIo(kCK->pkIO);
	}
}

void MyIOCP::handAccept(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred)
{
	MySharedCompleteKey kCK = pkOL->kSharedCk;
	bool bSucc = false;
	do 
	{
		if (IoResult != ERROR_SUCCESS)
		{
			printf("handAccept IoResult=%d\n", IoResult);
			break;
		}
		getSocketAddrs(_lpfnGetAcceptExSockAddrs,
			kCK,
			pkOL->kWSABuf.buf,
			(int)NumberOfBytesTransferred);

		if (setsockopt(kCK->uiSocket,
			SOL_SOCKET,
			SO_UPDATE_ACCEPT_CONTEXT,
			(const char*)&_ListenCK->uiSocket,
			sizeof(SOCKET)) == SOCKET_ERROR)
		{
			printf("handAccept setsockopt SO_UPDATE_ACCEPT_CONTEXT error=%d\n", 
				WSAGetLastError());
			break;
		}

		printf("ACCEPT: ip[%s], port[%u]----\n", kCK->pcRemoteIp, kCK->usRemotePort);

		int iError = reqRecv(kCK);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			printf("handAccept reqRecv fail iError=%d\n", iError);
			break;
		}
		bSucc = true;
	} while (0);

	if (!bSucc)
	{
		closesocket(kCK->uiSocket);
		CloseThreadpoolIo(kCK->pkIO);
	}

	delete pkOL;
	acceptOne();
}

void MyIOCP::initExtensionFuncPtr()
{
	SOCKET uiSocket = WSASocketW(
		AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP,
		NULL,
		0,
		WSA_FLAG_OVERLAPPED);
	if (SOCKET_ERROR == uiSocket)
	{
		exit(0);
	}
	GUID guid = WSAID_ACCEPTEX;
	getExtensionFuncPtr((LPVOID*)&_pfnAcceptEx, uiSocket, guid);
	guid = WSAID_DISCONNECTEX;
	getExtensionFuncPtr((LPVOID*)&_pfnDisconnectEx, uiSocket, guid);
	guid = WSAID_GETACCEPTEXSOCKADDRS;
	getExtensionFuncPtr((LPVOID*)&_lpfnGetAcceptExSockAddrs, uiSocket, guid);
}

int MyIOCP::reqSend(MySharedCompleteKey kCK, myBuffer& rSend)
{
	bool bWSSend = false;
	int iError = ERROR_SUCCESS;
	do
	{
		CLocalLock<CCriSec400> kLocker(kCK->ccsSend);
		kCK->iSendCnt++;
		if (kCK->iSendCnt == 1)
		{
			bWSSend = true;
			break;
		}
		kCK->kSendList.push_back(std::move(rSend));
	} while (0);
	if (bWSSend)
	{
		iError = postSend(kCK, rSend);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{

			CLocalLock<CCriSec400> kLocker(kCK->ccsSend);
			kCK->iSendCnt--;
		}
	}

	return iError;
}

int MyIOCP::reqRecv(MySharedCompleteKey kCK)
{
	return postRecv(kCK);
}



int MyIOCP::reqAccept(MySharedCompleteKey kCK)
{
	return postAccept(_pfnAcceptEx, _ListenCK, kCK);
}







