#include <Winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>


#pragma comment (lib, "WS2_32")
#pragma comment (lib, "Mswsock")
#pragma comment (lib, "Version")


extern SOCKET mtClientSocket(const char* pcIP, unsigned short usPort);

int mySend(SOCKET uiSocket, char* pcSend, int iLen)
{
	int iSend = 0;
	do 
	{
		int iRet = send(uiSocket, 
			pcSend + iSend, 
			iLen - iSend, 
			0);
		if (iRet == SOCKET_ERROR)
		{
			return SOCKET_ERROR;
		}
		iSend += iRet;
		if (iSend == iLen)
		{
			break;
		}
	} while (1);
	
	return iSend;
}

int myRecv(SOCKET uiSocket, char* pcRecv, int iSize)
{
	int iRet = recv(uiSocket, pcRecv, iSize, 0);
	return iRet;
}

int main()
{
	WSADATA kWSAData;
	int iRet = ::WSAStartup(MAKEWORD(2, 2), &kWSAData);
	if (iRet != 0)
	{
		return -1;
	}

	

	
	char pcSend[4096] = { 0 };
	char pcRecv[4096] = { 0 };
	
	do 
	{
		SOCKET uiSocket = mtClientSocket("127.0.0.1", 10055);
		memset(pcSend, 0, sizeof(pcSend));
		std::cin >> pcSend;
		iRet = mySend(uiSocket, pcSend, strnlen_s(pcSend, sizeof(pcSend)));
		if (iRet == SOCKET_ERROR)
		{
			std::cout << "send fail" << std::endl;
			closesocket(uiSocket);
			getchar();
		}
		memset(pcRecv, 0, sizeof(pcRecv));
		iRet = myRecv(uiSocket, pcRecv, sizeof(pcRecv));
		if (iRet == SOCKET_ERROR)
		{
			std::cout << "recv fail" << std::endl;
			closesocket(uiSocket);
			getchar();
		}
		else if (iRet == 0)
		{
			std::cout << "disconnect" << std::endl;
			closesocket(uiSocket);
			getchar();
		}
		std::cout << "recv:" << pcRecv << std::endl;
		closesocket(uiSocket);
		getchar();
	} while (1);
}







