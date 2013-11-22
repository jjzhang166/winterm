/*
 * remote.cpp
 *
 *  Created on: 2013-1-25
 *      Author: zhangbo
 */

#include <winsock2.h>

#include "types.h"
#include "remote.h"

unsigned int remote_open(const char* ip, int port) {
	WSADATA wsd;
	SOCKET sClient;
	struct sockaddr_in server;
	struct hostent *host = NULL;
	if (WSAStartup(MAKEWORD(2,2), &wsd) != 0) {
		return 0;
	}
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET) {
		return 0;
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(ip);
	if (server.sin_addr.s_addr == INADDR_NONE) {
		host = gethostbyname(ip);
		if (host == NULL) {
			return 0;
		}
		CopyMemory(&server.sin_addr, host->h_addr_list[0], host->h_length);
	}
	int m = connect(sClient, (struct sockaddr *) &server, sizeof(server));
	if (m == SOCKET_ERROR) {
		return 0;
	}

	return sClient;
}

unsigned int remote_recv(unsigned int handle, void* data, int maxlen) {
	int ret = 0;
	do {
		if (recv(handle, (char*) data, maxlen, 0) == 0) {
			break;
		}
	} while (FALSE);
	return ret;
}

unsigned int remote_send(unsigned int handle, const void* data, int len) {
	do {
		if (send(handle, (const char*) data, len, 0) == 0) {
			break;
		}
	} while (FALSE);

	return 0;
}

void remote_close(unsigned int sClient) {
	closesocket(sClient);
	WSACleanup();
}
