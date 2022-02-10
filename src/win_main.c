#ifdef WINDOWS

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 16384
#define DEFAULT_PORT "80"

#include "new_common.h"
#include "httpserver\new_http.h"
#include "new_pins.h"

void strcat_safe_test(){
	char tmpA[16];
	char tmpB[16];
	char buff[128];
	char timeStrA[128];
	char timeStrB[128];
	char timeStrC[128];
	char timeStrD[128];
	char timeStrE[128];
	char timeStrF[128];
	char timeStrG[128];
	char timeStrH[128];
	int res0, res1, res2, res3, res4, res5;
	tmpA[0] = 0;
	res0 = strcat_safe(tmpA,"Test1",sizeof(tmpA));
	res1 = strcat_safe(tmpA," ",sizeof(tmpA));
	res2 = strcat_safe(tmpA,"is now processing",sizeof(tmpA));
	res3 = strcat_safe(tmpA," very long string",sizeof(tmpA));
	res4 = strcat_safe(tmpA," and it",sizeof(tmpA));
	res5 = strcat_safe(tmpA," and it",sizeof(tmpA));
	tmpB[0] = 0;
	res0 = strcat_safe(tmpB,"Test1",sizeof(tmpB));
	res1 = strcat_safe(tmpB," ",sizeof(tmpB));
	res2 = strcat_safe(tmpB,"is now processing",sizeof(tmpB));
	res3 = strcat_safe(tmpB," very long string",sizeof(tmpB));
	res4 = strcat_safe(tmpB," and it",sizeof(tmpB));
	res5 = strcat_safe(tmpB," and it",sizeof(tmpB));

	urldecode2_safe(buff,"qqqqqq%40qqqq",sizeof(buff));
	urldecode2_safe(buff,"qqqqqq%40qqqq",sizeof(buff));

	
	misc_formatUpTimeString(15, timeStrA);
	misc_formatUpTimeString(65, timeStrB);
	misc_formatUpTimeString(125, timeStrC);
	misc_formatUpTimeString(60*60, timeStrD);
	misc_formatUpTimeString(4*60*60, timeStrE);
	misc_formatUpTimeString(24*60*60, timeStrF);
	misc_formatUpTimeString(24*60*60+60*60+50, timeStrG);
	misc_formatUpTimeString(100*24*60*60+60*60+15*60+50, timeStrH);
}
int Time_getUpTimeSeconds() {
	return rand()% 100000;
}
// placeholder - TODO
char myIP[] = "127.0.0.1";
char *getMyIp(){
    return myIP;
}

int __cdecl main(void) 
{
    WSADATA wsaData;
    int iResult;
	int len;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    char outbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    
	strcat_safe_test();

	PIN_ClearPins();
	PIN_SetPinChannelForPinIndex(1,1);
	PIN_SetPinRoleForPinIndex(1,IOR_Relay);

	PIN_SetPinChannelForPinIndex(2,2);
	PIN_SetPinRoleForPinIndex(2,IOR_PWM);
    init_rest();

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	while(1)
	{
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		

		// Receive until the peer shuts down the connection
		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				http_request_t request;
				memset(&request, 0, sizeof(request));

				recvbuf[iResult] = 0;

				request.fd = ClientSocket;
				request.received = recvbuf;
				request.receivedLen = iResult;
				outbuf[0] = '\0';
				request.reply = outbuf;
				request.replylen = 0;

				request.replymaxlen = DEFAULT_BUFLEN;

				printf("Bytes received: %d \n", iResult);
				len = HTTP_ProcessPacket(&request);

				if(len > 0) {
					printf("Bytes rremaining tosend %d\n", len);
					//printf("%s\n",outbuf);
					// Echo the buffer back to the sender
					//leen =  strlen(request.reply);
					iSendResult = send( ClientSocket, outbuf,len, 0 );
					if (iSendResult == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					printf("Bytes sent: %d\n", iSendResult);
				}
			}
			else if (iResult == 0)
				printf("Connection closing...\n");
			else  {
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			break;
		} while (iResult > 0);

		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			//WSACleanup();
			//return 1;
		}
	}

	// No longer need server socket
	closesocket(ListenSocket);

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

#endif

