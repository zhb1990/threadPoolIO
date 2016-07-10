#include <stdlib.h>
#include <string>
#include <map>
#include <memory>
#include <time.h>
#include <iostream>

#include <jemalloc/jemalloc.h>
#include <cmdline.h>

#include "myAllocator.h"

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

typedef std::basic_stringstream<char, std::char_traits<char>, MyAlloc<char>> myStringStream;
void mtPrint(void* pkStream, const char* pcStr)
{
	(*(myStringStream*)pkStream) << pcStr;
}
int main(int argc, char **argv)
{
	myStringStream kString;
	je_malloc_stats_print(&mtPrint, &kString, NULL);

	std::basic_string<char, std::char_traits<char>, MyAlloc<char>> mystring = kString.str();
	std::cout << "len" << mystring.length() << std::endl;

	je_malloc_stats_print(NULL, NULL, NULL);
	std::vector<myBuffer, MyAlloc<myBuffer>> vecTest;
	myBuffer kCK;
	vecTest.push_back(kCK);
	

	je_malloc_stats_print(NULL, NULL, NULL);

	vecTest.clear();
	vecTest.shrink_to_fit();

	je_malloc_stats_print(NULL, NULL, NULL);

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















