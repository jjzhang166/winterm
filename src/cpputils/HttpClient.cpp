/*
 * HttpClient.cpp
 *
 *  Created on: 2012-11-27
 *      Author: zhangbo
 */

#include "HttpClient.h"

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include "Util.h"

HttpClient::HttpClient() {
	// TODO Auto-generated constructor stub

}

HttpClient::HttpClient(const string& host, int port, const string& url) {
	this->listener = NULL;
	this->host = host;
	this->port = port;
	this->url = url;
	this->timeout = 20 * 1000;
}

HttpClient::~HttpClient() {
	// TODO Auto-generated destructor stub
}

void HttpClient::SetHost(const string& host) {
	this->host = host;
}

void HttpClient::SetFile(const string& file) {
	this->file = file;
}

string HttpClient::GetFile() {
	return file;
}

void HttpClient::SetId(const string& id) {
	this->id = id;
}

string HttpClient::GetId() {
	return id;
}

string HttpClient::GetHost() {
	return this->host;
}

void HttpClient::SetPort(int port) {
	this->port = port;
}

void HttpClient::SetTimeout(unsigned long timeout) {
	this->timeout = timeout;
}

int HttpClient::GetPort() {
	return this->port;
}

unsigned long HttpClient::GetTimeout() {
	return this->timeout;
}

void HttpClient::SetUrl(const string& url) {
	this->url = url;
}

string HttpClient::GetUrl() {
	return this->url;
}

HttpClient& HttpClient::SetListener(DataReceiver* listener) {
	this->listener = listener;
	return *this;
}

#define HTTP_MULTIPART_BOUNDARY "----WebKitFormBoundaryzcAM1pzZxGjJ6Q4c"

#define HTTP_MULTIPART_HEADER "\
POST %s HTTP/1.1\r\n\
HOST: %s:%d\r\n\
Accept: */\
*\r\n\
Content-Length: %d\r\n\
Content-Type: multipart/form-data; boundary=%s\r\n"

#define HTTP_MULTIPART_FILE_FORM_DATA_HEADER "\
\r\n--%s\r\n\
Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n\
Content-Type: application/octet-stream\r\n\r\n"

#define HTTP_MULTIPART_FORM_DATA_HEADER "\
\r\n--%s\r\n\
Content-Disposition: form-data; name=\"%s\"\r\n\r\n"

#define HTTP_MULTIPART_FORM_DATA "\
\r\n--%s\r\n\
Content-Disposition: form-data; name=\"%s\"\r\n\r\n%s"

#define HTTP_MULTIPART_END "\r\n--%s--\r\n"

int HttpClient::Post() {
	WSADATA wsd;
	SOCKET sClient;
	int ret;
	struct sockaddr_in server;
	struct hostent *host = NULL;
	if (WSAStartup(MAKEWORD(2,2), &wsd) != 0) {
		printf("failed to load winsock library!/n");
		return 1;
	}
	sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sClient == INVALID_SOCKET) {
		printf("socket() fialed: %d/n", WSAGetLastError());
		return 1;
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(this->host.c_str());
	if (server.sin_addr.s_addr == INADDR_NONE) {
		host = gethostbyname(this->host.c_str());
		if (host == NULL) {
			printf("Unable to resolve server:%s/n", this->host.c_str());
			return 1;
		}
		CopyMemory(&server.sin_addr, host->h_addr_list[0], host->h_length);
	}
	int m = connect(sClient, (struct sockaddr *) &server, sizeof(server));
	if (m == SOCKET_ERROR) {
		int errorCode = WSAGetLastError();
		printf("connect() fialed:%d/n", errorCode);
		return 1;
	}

	char szFileHeader[256];
	sprintf(szFileHeader, HTTP_MULTIPART_FILE_FORM_DATA_HEADER,
			HTTP_MULTIPART_BOUNDARY, "file", this->file.c_str());

	char szFormMethod[256];
	sprintf(szFormMethod, HTTP_MULTIPART_FORM_DATA, HTTP_MULTIPART_BOUNDARY,
			"method", "upload");

	char szFormId[256];
	sprintf(szFormId, HTTP_MULTIPART_FORM_DATA, HTTP_MULTIPART_BOUNDARY, "id",
			this->id.c_str());

	char szEnd[256];
	sprintf(szEnd, HTTP_MULTIPART_END, HTTP_MULTIPART_BOUNDARY);

	unsigned long length = Util::GetSize(file);
	unsigned long contentLength = length + strlen(szFileHeader)
			+ strlen(szFormMethod) + strlen(szFormId) + strlen(szEnd) - 2;

	char szHeader[1024];
	sprintf(szHeader, HTTP_MULTIPART_HEADER, this->url.c_str(),
			this->host.c_str(), this->port, contentLength,
			HTTP_MULTIPART_BOUNDARY);

	do {
		if (send(sClient, szHeader, strlen(szHeader), 0) == 0)
			break;

		if (send(sClient, szFileHeader, strlen(szFileHeader), 0) == 0)
			break;

		void* buffer = malloc(length);
		FILE* pf = fopen(file.c_str(), "rb");
		fread(buffer, length, 1, pf);
		if (send(sClient, (char*) buffer, length, 0) == 0)
			break;
		free(buffer);
		fclose(pf);

		if (send(sClient, szFormMethod, strlen(szFormMethod), 0) == 0)
			break;

		if (send(sClient, szFormId, strlen(szFormId), 0) == 0)
			break;

		if (send(sClient, szEnd, strlen(szEnd), 0) == 0)
			break;

		int DEFAULT_BUFFER = 1024;
		char szBuffer[DEFAULT_BUFFER];
		ret = recv(sClient, szBuffer, DEFAULT_BUFFER, 0);
		if (ret == 0)
			break;
		else if (ret == SOCKET_ERROR) {
			printf("recv() fialed:%d/n", WSAGetLastError());
			break;
		}
	} while (false);

	closesocket(sClient);
	WSACleanup();
	return 0;
}
