#ifdef WINDOWS

#include "../new_common.h"
#include "lwip/sockets.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "../logging/logging.h"
#include "new_http.h"
#include <timeapi.h>

 SOCKET ListenSocket = INVALID_SOCKET;

int g_httpPort = 80;

int HTTPServer_Start() {

	int iResult;
	int argp;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

	if (ListenSocket != INVALID_SOCKET) {
		closesocket(ListenSocket);
	}
    // Resolve the server address and port
	char service[6];
	snprintf(service, sizeof(service), "%u", g_httpPort);

	iResult = getaddrinfo(NULL, service, &hints, &result);
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

    argp = 1;
    if (ioctlsocket(ListenSocket,
        FIONBIO,
        &argp) == SOCKET_ERROR)
    {
        printf("ioctlsocket() error %d\n", WSAGetLastError());
        return 1;
    }
}
#define DEFAULT_BUFLEN 10000
int g_prevHTTPResult;

void HTTPServer_RunQuickTick() {
	int iResult;
	int err;
    char recvbuf[DEFAULT_BUFLEN];
    char outbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    SOCKET ClientSocket = INVALID_SOCKET;
	int len, iSendResult;

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		iResult = WSAGetLastError();
		if(iResult != WSAEWOULDBLOCK) {
			if (iResult != g_prevHTTPResult) {
				printf("accept failed with error: %d\n", iResult);
				g_prevHTTPResult = iResult;
			}
		}
		return;
	}

	//argp = 0;
	//if (ioctlsocket(ClientSocket,
	//	FIONBIO,
	//	&argp) == SOCKET_ERROR)
	//{
	//	printf("ioctlsocket() error %d\n", WSAGetLastError());
	//	return 1;
	//}
		// Receive until the peer shuts down the connection
		do {

			iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				http_request_t request;
				memset(&request, 0, sizeof(request));

				recvbuf[iResult] = 0;

#if 1
				// debug test code, you can disable it but dont remove it
				if (1) {
					FILE *f;

					f = fopen("lastHTTPPacket.txt", "wb");
					fwrite(recvbuf, 1, recvbuflen, f);
					fclose(f);
				}
#endif

				request.fd = ClientSocket;
				request.received = recvbuf;
				request.receivedLen = iResult;
				outbuf[0] = '\0';
				request.reply = outbuf;
				request.replylen = 0;
				request.responseCode = HTTP_RESPONSE_OK;
				request.replymaxlen = DEFAULT_BUFLEN;

				//printf("HTTP Server for Windows: Bytes received: %d \n", iResult);
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
						return 1;
					}
					printf("HTTP Server for Windows: Bytes sent: %d\n", iSendResult);
				}
				break;
			}
			else if (iResult == 0) {
				printf("Connection closing...\n");
				break;
			}
			else {
				err = WSAGetLastError();
				if (err != WSAEWOULDBLOCK) {
					printf("recv failed with error: %d\n", err);
					closesocket(ClientSocket);
					break;
				}
			}
			break;
		} while (1);
		//byte zero = 0;
		//send(ClientSocket, &zero, 0, 0);
		//Sleep(50);
		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		//long firstAttempt = timeGetTime();
		//while (1) {
		//	iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		//	if (iResult == 0)
		//		break;
		//	err = WSAGetLastError();
		//	if (err != WSAEWOULDBLOCK) {
		//		break;
		//	}
		//	long delta = timeGetTime() - firstAttempt;
		//	if (delta > 2) {
		//		printf("HTTP server would freeze to long!\n");
		//		break; // too long freeze!

		//	}
		//}
		//Sleep(50);
		//iResult = closesocket(ClientSocket);
		//if (iResult == SOCKET_ERROR) {
		///	printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			//WSACleanup();
			//return 1;
		//}
}

#endif

