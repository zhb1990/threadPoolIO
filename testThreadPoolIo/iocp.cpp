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
		PMyCompleteKey pkCK = pkOL->pkCk;
		if (pkCK == NULL)
		{
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
	:_usListenPort(usListenPort)
{
	initExtensionFuncPtr();
	int iError = ERROR_SUCCESS;
	SOCKET uiListenSocket = createListenSocket(_usListenPort, iError);
	if (uiListenSocket == SOCKET_ERROR)
	{
		exit(0);
	}

	_ListenCK.uiSocket = uiListenSocket;
	_ListenCK.pkIO = CreateThreadpoolIo((HANDLE)_ListenCK.uiSocket, 
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
		PMyCompleteKey pkCK = new MyCompleteKey();
		if (pkCK == NULL)
		{
			closesocket(uiSocket);
			break;
		}
		pkCK->uiSocket = uiSocket;
		pkCK->pkIO = CreateThreadpoolIo((HANDLE)pkCK->uiSocket,
			(PTP_WIN32_IO_CALLBACK)IoCompletionCallback,
			this,
			NULL);
		if (pkCK->pkIO == NULL)
		{
			closesocket(uiSocket);
			delete pkCK;
			break;
		}
		iError = reqAccept(pkCK);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			//closesocket(uiSocket);
			CloseThreadpoolIo(pkCK->pkIO);
			delete pkCK;
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

int MyIOCP::mySend(PMyCompleteKey pkCK, myBuffer kSend)
{
	int iRet = FALSE;
	do
	{
		if (kSend.size() <= 0)
		{
			break;
		}
		int iError = reqSend(pkCK, std::move(kSend));
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			printf("SEND: FAIL [%s] -> ip[%s], port[%u], reqSend error=%d\n", kSend.data(),
				pkCK->pcRemoteIp, pkCK->usRemotePort, iError);

			closesocket(pkCK->uiSocket);
			CloseThreadpoolIo(pkCK->pkIO);
			delete pkCK;
			break;
		}
		iRet = TRUE;
	} while (0);

	return iRet;
}
int MyIOCP::mySend(PMyCompleteKey pkCK, char* pcSend, int iLen)
{
	int iRet = FALSE;
	do 
	{
		if (iLen <= 0)
		{
			break;
		}
		iRet = mySend(pkCK, std::move(myBuffer(pcSend, iLen)));
	} while (0);
	
	return iRet;
}


void MyIOCP::handSend(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred)
{
	PMyCompleteKey pkCK = pkOL->pkCk;
	bool bSucc = false;
	do 
	{
		if (IoResult != ERROR_SUCCESS)
		{
			printf("SEND: FAIL [%s] -> ip[%s], port[%u], IoResult=%d\n", pkOL->kBuffer.data(),
				pkCK->pcRemoteIp, pkCK->usRemotePort, IoResult);
			break;
		}

		printf("SEND:[%s] -> ip[%s], port[%u]\n", pkOL->kBuffer.data(),
			pkCK->pcRemoteIp, pkCK->usRemotePort);

		myBuffer kBuffer;
		{
			CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
			pkCK->iSendCnt--;
			if (pkCK->kSendList.size() > 0)
			{
				kBuffer = std::move(pkCK->kSendList.front());
				pkCK->kSendList.pop_front();
			}
		}
		if (kBuffer.size() > 0)
		{
			int iError = postSend(pkCK, std::move(kBuffer));
			if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
			{
				printf("SEND: FAIL [%s] -> ip[%s], port[%u], postSend error=%d\n", kBuffer.data(),
					pkCK->pcRemoteIp, pkCK->usRemotePort, iError);
				break;
			}
		}
		bSucc = true;
	} while (0);
	
	delete pkOL;
	if (!bSucc)
	{
		closesocket(pkCK->uiSocket);
		CloseThreadpoolIo(pkCK->pkIO);
		delete pkCK;
	}
}

void MyIOCP::handRecv(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred)
{
	PMyCompleteKey pkCK = pkOL->pkCk;
	bool bSucc = false;

	do
	{
		if (NumberOfBytesTransferred == 0)
		{
			printf("Active disconnect: ip[%s], port[%u]\n",
				pkCK->pcRemoteIp, pkCK->usRemotePort);
			break;
		}

		if (IoResult != ERROR_SUCCESS)
		{
			printf("RECV: FAIL ip[%s], port[%u], IoResult=%d\n",
				pkCK->pcRemoteIp, pkCK->usRemotePort, IoResult);
			break;
		}

		pkOL->kBuffer.hasWritten(NumberOfBytesTransferred);
		printf("RECV:[%s] <- ip[%s], port[%u]\n", pkOL->kBuffer.data(),
			pkCK->pcRemoteIp, pkCK->usRemotePort);

		mySend(pkCK, std::move(pkOL->kBuffer));

		int iError = reqRecv(pkCK);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			printf("handRecv reqRecv fail ip[%s], port[%u], iError=%d\n", 
				pkCK->pcRemoteIp, pkCK->usRemotePort, iError);
			break;
		}
		
		bSucc = true;
	} while (0);

	delete pkOL;
	if (!bSucc)
	{
		closesocket(pkCK->uiSocket);
		CloseThreadpoolIo(pkCK->pkIO);
		delete pkCK;
	}
}

void MyIOCP::handAccept(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred)
{
	PMyCompleteKey pkCK = pkOL->pkCk;
	bool bSucc = false;
	do 
	{
		if (IoResult != ERROR_SUCCESS)
		{
			printf("handAccept IoResult=%d\n", IoResult);
			break;
		}
		getSocketAddrs(_lpfnGetAcceptExSockAddrs,
			pkCK,
			pkOL->kWSABuf.buf,
			NumberOfBytesTransferred);

		if (setsockopt(pkCK->uiSocket,
			SOL_SOCKET,
			SO_UPDATE_ACCEPT_CONTEXT,
			(const char*)&_ListenCK.uiSocket,
			sizeof(SOCKET)) == SOCKET_ERROR)
		{
			printf("handAccept setsockopt SO_UPDATE_ACCEPT_CONTEXT error=%d\n", 
				WSAGetLastError());
			break;
		}

		printf("ACCEPT: ip[%s], port[%u]----\n", pkCK->pcRemoteIp, pkCK->usRemotePort);

		int iError = reqRecv(pkCK);
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{
			printf("handAccept reqRecv fail iError=%d\n", iError);
			break;
		}
		bSucc = true;
	} while (0);

	if (!bSucc)
	{
		closesocket(pkCK->uiSocket);
		CloseThreadpoolIo(pkCK->pkIO);
		delete pkCK;
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

int MyIOCP::reqSend(PMyCompleteKey pkCK, myBuffer kSend)
{
	bool bWSSend = false;
	int iError = ERROR_SUCCESS;
	do
	{
		CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
		pkCK->iSendCnt++;
		if (pkCK->iSendCnt == 1)
		{
			bWSSend = true;
			break;
		}
		pkCK->kSendList.push_back(std::move(kSend));
	} while (0);
	if (bWSSend)
	{
		iError = postSend(pkCK, std::move(kSend));
		if (iError != ERROR_SUCCESS && iError != WSA_IO_PENDING)
		{

			CLocalLock<CCriSec400> kLocker(pkCK->ccsSend);
			pkCK->iSendCnt--;
		}
	}

	return iError;
}

int MyIOCP::reqRecv(PMyCompleteKey pkCK)
{
	return postRecv(pkCK);
}



int MyIOCP::reqAccept(PMyCompleteKey pkCK)
{
	return postAccept(_pfnAcceptEx, &_ListenCK, pkCK);
}







