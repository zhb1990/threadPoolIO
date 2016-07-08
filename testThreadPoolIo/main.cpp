#include <stdlib.h>
#include <string>
#include <map>
#include <memory>
#include <time.h>
#include "iocp.h"
#include "lock.h"

#pragma comment (lib, "WS2_32")
#pragma comment (lib, "Mswsock")
#pragma comment (lib, "Version")

typedef BOOLEAN(_stdcall* RtlGenRandomFunc)(void * RandomBuffer, ULONG RandomBufferLength);
RtlGenRandomFunc RtlGenRandom = NULL;

void initRandom()
{
	if (RtlGenRandom == NULL) {
		// Load proc if not loaded
		HMODULE lib = LoadLibraryA("advapi32.dll");
		RtlGenRandom = (RtlGenRandomFunc)GetProcAddress(lib, "SystemFunction036");
	}
}


int replace_random() {
	unsigned int x = 0;
	if (RtlGenRandom == NULL)
	{
		return 0;
	}
	RtlGenRandom(&x, sizeof(unsigned int));
	return (int)(x >> 1);
}

inline int rand1(int data)	  //[0,i)   =[0,i-1]
{
	if (data == 0)
	{
		return 0;
	}

	return replace_random() % data;
}

inline int rand2(int data)	  //[0,i)   =[0,i-1]
{
	if (data == 0)
	{
		return 0;
	}

	return (int)(((double)rand() / RAND_MAX) * data);
}

inline int rand3(int data)	  //[0,i)   =[0,i-1]
{
	if (data == 0)return 0;
	return (int)(((__int64)(((rand() & 0x7fff) << 15)) | (rand() & 0x7fff))) % data;
}

int main(int argc, char **argv)
{
	WSADATA kWSAData;
	int iRet = ::WSAStartup(MAKEWORD(2, 2), &kWSAData);
	if (iRet != 0)
	{
		return -1;
	}

	MyIOCP kIOCP(10055);
	kIOCP.startAccept();

	kIOCP.myConnect("127.0.0.1", 10055);

	SleepEx(INFINITE, FALSE);
}















