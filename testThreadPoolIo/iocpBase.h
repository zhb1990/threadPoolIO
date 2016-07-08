#pragma once

#include <list>
#include <vector>
#include <utility>
#include <algorithm>
#include <iostream>
#include <memory>

#include <Winsock2.h>
#include <mswsock.h>
#include "lock.h"

typedef unsigned long CONNID;

extern volatile long lCntBufferCreate;
extern volatile long lCntBufferRelease;

extern volatile long lCnt_CompleteKeyCreate;
extern volatile long lCnt_CompleteKeyRelease;

extern volatile long lCnt_OverlappedCreate;
extern volatile long lCnt_OverlappedRelease;

#define MY_PAGE_NUM 4096 //4k
class myBuffer
{
public:
	myBuffer()
		:_buf(MY_PAGE_NUM)
	{
		_readPos = 0;
		_writePos = 0;
		long lCnt = InterlockedIncrement(&lCntBufferCreate);
		std::cout << "myBuffer :" << lCnt << std::endl;
	}

	myBuffer(const char* pcBuf, int iLen)
		:_buf(MY_PAGE_NUM)
	{
		_readPos = 0;
		_writePos = 0;
		append(pcBuf, iLen);
		long lCnt = InterlockedIncrement(&lCntBufferCreate);
		std::cout << "myBuffer :" << lCnt << std::endl;
	}
	myBuffer(myBuffer& buffer)
		:_buf(MY_PAGE_NUM)
	{
		_readPos = buffer._readPos;
		_writePos = buffer._writePos;
		_buf = buffer._buf;
		long lCnt = InterlockedIncrement(&lCntBufferCreate);
		std::cout << "myBuffer :" << lCnt << std::endl;
	}

	myBuffer(myBuffer&& buffer) noexcept
		:_buf(MY_PAGE_NUM)
	{
		_readPos = 0;
		_writePos = 0;
		swap(buffer);
		long lCnt = InterlockedIncrement(&lCntBufferCreate);
		std::cout << "myBuffer :" << lCnt << std::endl;
	}

	~myBuffer()
	{
		long lCnt = InterlockedDecrement(&lCntBufferCreate);
		std::cout << "~myBuffer :" << lCnt << std::endl;
	}

	void reset()
	{
		_buf.clear();
		_readPos = 0;
		_writePos = 0;
	}

	void swap(myBuffer& kRight)
	{
		_buf.swap(kRight._buf);
		std::swap(_readPos, kRight._readPos);
		std::swap(_writePos, kRight._writePos);
	}

	myBuffer& operator= (myBuffer&& kRight) noexcept
	{
		if (this != &kRight)
		{
			swap(kRight);
		}
		return *this;
	}

	myBuffer& operator= (myBuffer& kRight)
	{
		if (this != &kRight)
		{
			_readPos = kRight._readPos;
			_writePos = kRight._writePos;
			_buf = kRight._buf;
		}
		return *this;
	}

	void append(const char* pcBuf, int iLen)
	{
		ensureWritableBytes(iLen);
		std::_Copy_no_deprecate(pcBuf, pcBuf + iLen, beginWrite());
		hasWritten(iLen);
	}

	char* beginWrite()
	{
		return begin() + _writePos;
	}

	char* beginRead()
	{
		return begin() + _readPos;
	}

	int size()
	{
		return _writePos - _readPos;
	}

	int writableBytes() const
	{
		return (int)_buf.size() - _writePos;
	}

	void hasWritten(int iLen)
	{
		_writePos += iLen;
	}

private:

	void ensureWritableBytes(int iLen)
	{
		if (writableBytes() < iLen)
		{
			makeSpace(iLen);
		}
	}

	void makeSpace(int iLen)
	{
		if (writableBytes() + _readPos < iLen)
		{
			_buf.resize(_writePos + iLen);
		}
		else
		{
			int readable = size();
			std::_Copy_no_deprecate(begin() + _readPos,
				begin() + _writePos,
				begin());
			_readPos = 0;
			_writePos = _readPos + readable;
		}
	}

	char* begin()
	{
		return &*_buf.begin();
	}

	const char* begin() const
	{
		return &*_buf.begin();
	}

protected:
	std::vector<char> _buf;
	int _readPos;
	int _writePos;
};

typedef std::list<myBuffer> MyBufferList;

typedef struct _CompleteKey
{
	SOCKET uiSocket;		/// 套接字
	PTP_IO pkIO;			/// 绑定到 ThreadpoolIO 的对象
	unsigned short usRemotePort;	/// 远程端口
	unsigned short usLocalPort;	/// 本地端口
	char pcRemoteIp[16];	/// 远程ip
	char pcLocalIp[16];	/// 本地ip

	CCriSec400 ccsSend;
	int iSendCnt;
	MyBufferList kSendList;

	_CompleteKey()
	{
		memset(pcRemoteIp, 0, sizeof(pcRemoteIp));
		memset(pcLocalIp, 0, sizeof(pcLocalIp));
		iSendCnt = 0;
		usRemotePort = 0;
		usLocalPort = 0;
		uiSocket = SOCKET_ERROR;
		pkIO = NULL;

		long lCnt = InterlockedIncrement(&lCnt_CompleteKeyCreate);
		std::cout << "_CompleteKey :" << lCnt << std::endl;
	}
	~_CompleteKey()
	{
		long lCnt = InterlockedDecrement(&lCnt_CompleteKeyCreate);
		std::cout << "~_CompleteKey :" << lCnt << std::endl;
	}

	void reset()
	{
		memset(pcRemoteIp, 0, sizeof(pcRemoteIp));
		memset(pcLocalIp, 0, sizeof(pcLocalIp));
		iSendCnt = 0;
		usRemotePort = 0;
		usLocalPort = 0;
		kSendList.clear();
	}
} MyCompleteKey;

typedef std::shared_ptr<MyCompleteKey> MySharedCompleteKey;
//typedef std::map<CONNID, PMyCompleteKey> CKPtrMap;

enum EOverlappedType /// 操作类型
{
	E_OLT_ACCEPTEX = 0,
	E_OLT_CONNECTEX,
	E_OLT_WSARECV,
	E_OLT_WSASEND,
	E_OLT_WSADISCONNECTEX,
	E_OLT_UNKNOW
};

typedef struct _Overlapped
{
	OVERLAPPED kOverlapped;
	EOverlappedType eType;
	WSABUF kWSABuf;
	myBuffer kBuffer;
	MySharedCompleteKey kSharedCk;

	_Overlapped()
	{
		memset(&kOverlapped, 0, sizeof(kOverlapped));
		memset(&kWSABuf, 0, sizeof(kWSABuf));
		long lCnt = InterlockedIncrement(&lCnt_OverlappedCreate);
		std::cout << "_Overlapped :" << lCnt << std::endl;
	}

	~_Overlapped()
	{
		long lCnt = InterlockedDecrement(&lCnt_OverlappedCreate);
		std::cout << "~_Overlapped :" << lCnt << std::endl;
	}
} MyOverlapped, *PMyOverlapped;


// 获取 Socket 的某个扩展函数的指针
int getExtensionFuncPtr(LPVOID* plpf, SOCKET sock, GUID guid);

int postSend(MySharedCompleteKey kSharedCk, myBuffer& rSend);
int postAccept(LPFN_ACCEPTEX pfnAcceptEx, 
	MySharedCompleteKey kSharedCkListen, MySharedCompleteKey kSharedCk);
int postRecv(MySharedCompleteKey kSharedCk);
int postDisconnect(LPFN_DISCONNECTEX pfnDisconnectEx, MySharedCompleteKey kSharedCk);
int postConnect(LPFN_CONNECTEX pfnConnectEx, 
	MySharedCompleteKey kSharedCk, 
	const SOCKADDR* name,
	int namelen);
void getSocketAddrs(LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs,
	MySharedCompleteKey kSharedCk,
	char* pAcceptInfo,
	int iRecvBytes);



