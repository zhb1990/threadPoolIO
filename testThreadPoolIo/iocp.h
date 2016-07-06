#pragma once

#include "iocpBase.h"


class MyIOCP
{
public:
	MyIOCP(unsigned short usListenPort);

	void startAccept();
	int acceptOne();
	
	int mySend(PMyCompleteKey pkCK, char* pcSend, int iLen);
	int mySend(PMyCompleteKey pkCK, myBuffer kSend);

	friend VOID CALLBACK IoCompletionCallback(PTP_CALLBACK_INSTANCE Instance,
		PVOID Context,
		PVOID Overlapped,
		ULONG IoResult,
		ULONG_PTR NumberOfBytesTransferred,
		PTP_IO Io);
protected:
	void handSend(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred);
	void handRecv(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred);
	void handAccept(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred);


	void initExtensionFuncPtr();
	int reqSend(PMyCompleteKey pkCK, myBuffer kSend);
	int reqAccept(PMyCompleteKey pkCK);
	int reqRecv(PMyCompleteKey pkCK);
	
	SOCKET createListenSocket(unsigned short usListenPort, int& iError);
	SOCKET createAcceptSocket(int& iError);
private:
	LPFN_ACCEPTEX _pfnAcceptEx;
	LPFN_DISCONNECTEX _pfnDisconnectEx;
	LPFN_GETACCEPTEXSOCKADDRS _lpfnGetAcceptExSockAddrs;

	MyCompleteKey _ListenCK;
	unsigned short _usListenPort;
};
























