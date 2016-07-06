#include <stdlib.h>
#include <string>
#include <map>
#include <memory>

#include "iocp.h"
#include "lock.h"

#pragma comment (lib, "WS2_32")
#pragma comment (lib, "Mswsock")
#pragma comment (lib, "Version")



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

	SleepEx(INFINITE, FALSE);
}















