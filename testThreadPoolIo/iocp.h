#pragma once

#include "iocpBase.h"


class MyIOCP
{
public:
	MyIOCP(unsigned short usListenPort);

	void startAccept();
	int acceptOne();
	MySharedCompleteKey myConnect(char* pcIP, unsigned short usPort);
	
	int mySend(MySharedCompleteKey kCK, char* pcSend, int iLen);
	int mySend(MySharedCompleteKey kCK, myBuffer& rSend);

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
	void handConnect(ULONG IoResult, PMyOverlapped pkOL, ULONG_PTR NumberOfBytesTransferred);

	void initExtensionFuncPtr();
	int reqSend(MySharedCompleteKey kCK, myBuffer& rSend);
	int reqAccept(MySharedCompleteKey kCK);
	int reqRecv(MySharedCompleteKey kCK);
	int reqConnect(MySharedCompleteKey kCK, char* pcIP, unsigned short usPort);
	
	SOCKET createListenSocket(unsigned short usListenPort, int& iError);
	SOCKET createUISocket(int& iError);
private:
	LPFN_ACCEPTEX _pfnAcceptEx;
	LPFN_DISCONNECTEX _pfnDisconnectEx;
	LPFN_GETACCEPTEXSOCKADDRS _lpfnGetAcceptExSockAddrs;
	LPFN_CONNECTEX _pfnConnectEx;


	MySharedCompleteKey _ListenCK;
	unsigned short _usListenPort;
};
























