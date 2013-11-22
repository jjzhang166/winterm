#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <stdio.h>
#include "HttpRequest.h"
#include "Logger.h"
#include <wininet.h>
#include <iostream>

Logger* HttpRequest::log = NULL;

HttpRequest::HttpRequest(void) {
	this->listener = NULL;
	this->timeout = 20 * 1000;
}

HttpRequest::HttpRequest(const string& host, int port, const string& url) {
	this->listener = NULL;
	this->host = host;
	this->port = port;
	this->url = url;
	this->timeout = 20 * 1000;
}

HttpRequest::~HttpRequest(void) {

}

HttpRequest& HttpRequest::Append(const string& name, const string& value) {
	this->Put(name, value);
	return *this;
}

HttpRequest& HttpRequest::SetRequestParameter(RequestParameter& param) {
	this->SetHost(param.GetHost());
	this->SetPort(param.GetPort());
	this->SetUrl(param.GetUrl());
	this->SetTimeout(param.GetTimeout());
	this->SetParameters(param);
	return *this;
}

void HttpRequest::SetHost(const string& host) {
	this->host = host;
}

string HttpRequest::GetHost() {
	return this->host;
}

void HttpRequest::SetPort(int port) {
	this->port = port;
}

void HttpRequest::SetTimeout(unsigned long timeout) {
	this->timeout = timeout;
}

int HttpRequest::GetPort() {
	return this->port;
}

unsigned long HttpRequest::GetTimeout() {
	return this->timeout;
}

void HttpRequest::SetUrl(const string& url) {
	this->url = url;
}

string HttpRequest::GetUrl() {
	return this->url;
}

HttpRequest& HttpRequest::SetListener(DataReceiver* listener) {
	this->listener = listener;
	return *this;
}

bool HttpRequest::Get(int times) {
	return Process(times, "get");
}

bool HttpRequest::Post(int times) {
	return Process(times, "post");
}

bool HttpRequest::Process(int times, string method) {
	bool success = false;
	int i = 0;
	while (!success && i++ < times) {
		if (method.compare("post") == 0) {
			success = this->Post();
		} else if (method.compare("get") == 0) {
			success = this->Get();
		}
	}
	if (!success) {
		return false;
	}
	return true;
}

void HttpRequest::Log(const string& url) {
	if (log == NULL) {
		log = new Logger("C:\\Program Files\\bank\\urls.log");
		log->SetLevel(Logger::LOG_DEBUG);
	}
	log->Debug(url);
}

#define BUFFER_LENGTH 1024*1024

bool HttpRequest::Get() {
	bool ret = false;
	char buffer[BUFFER_LENGTH];
	DWORD bytes_read;
	HINTERNET hSession = InternetOpen("HTTP Downloader",
			INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (!hSession) {
		return ret;
	}
	if (!InternetSetOption(hSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout,
			sizeof(DWORD))) {
		InternetCloseHandle(hSession);
		return ret;
	}
	char strPort[6];
	sprintf(strPort, "%d", this->port);
	string para = this->GetParameters();
	if (para != "") {
		para = "?" + para;
	}
	string fullurl = "http://" + host + ":" + strPort + url + para;
	CHAR szHead[] = "Accept: */*\r\n\r\n";
	HINTERNET hOpenUrl = InternetOpenUrl(hSession, fullurl.c_str(), szHead,
			lstrlen(szHead), INTERNET_FLAG_DONT_CACHE, 0);
	if (!hOpenUrl) { // | INTERNET_FLAG_KEEP_CONNECTION INTERNET_FLAG_DONT_CACHE
		InternetCloseHandle(hSession);
		return ret;
	}

	DWORD dwStatus;
	DWORD dwBuffLen = sizeof(dwStatus);
	BOOL bSuccess = HttpQueryInfo(hOpenUrl,
			HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus,
			&dwBuffLen, 0);

	Log(fullurl);
	if (bSuccess && dwStatus == 200) {
		do {
			if (!InternetReadFile(hOpenUrl, buffer, BUFFER_LENGTH,
					&bytes_read)) {
				ret = true;
				break;
			}
			if (bytes_read > 0) {
				ret = true;
				if (this->listener != NULL) {
					if (!this->listener->OnReceive(bytes_read, buffer)) {
						ret = false;
						break;
					}
				}
			} else {
				ret = true;
				break;
			}
		} while (true);
	}

	InternetCloseHandle(hOpenUrl);
	InternetCloseHandle(hSession);

	return ret;
}

string HttpRequest::GetParameters() {
	string post;
	map<string, string>::iterator it;
	for (it = this->Begin(); it != this->End(); it++) {
		if (it != this->Begin()) {
			post += "&";
		}
		post += it->first + "=" + it->second;
	}
	return post;
}

bool HttpRequest::Post() {
	HINTERNET pServer, pSession;
	pSession = InternetOpen("seantest", INTERNET_OPEN_TYPE_PRECONFIG, NULL,
			NULL, 0);
	pServer = InternetConnect(pSession, this->host.c_str(),
	/*INTERNET_INVALID_PORT_NUMBER*/this->port, NULL, NULL,
			INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_CACHE_WRITE, 0);

	INTERNET_BUFFERS BufferIn = { 0 };
	DWORD dwBytesWritten;

	HINTERNET hRequest = HttpOpenRequest(pServer, "POST", this->url.c_str(),
			NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE, 0);

	if (!hRequest) {
		return false;
	}

	string post = this->GetParameters();

	BufferIn.dwStructSize = sizeof(INTERNET_BUFFERS);
	BufferIn.dwBufferTotal = post.size();
	BufferIn.dwBufferLength = 0;
	BufferIn.lpcszHeader =
			"Content-Type: application/x-www-form-urlencoded\r\n";
	BufferIn.dwHeadersLength = strlen(BufferIn.lpcszHeader);
	BufferIn.dwHeadersTotal = 0;
	BufferIn.lpvBuffer = NULL;
	BufferIn.Next = NULL;
	BufferIn.dwOffsetHigh = 0;
	BufferIn.dwOffsetLow = 0;

	if (!HttpSendRequestEx(hRequest, &BufferIn, NULL, 0, 0)) {
		return false;
	}

	if (!InternetWriteFile(hRequest, post.c_str(), post.length(),
			&dwBytesWritten)) {
		return false;
	}

	if (!HttpEndRequest(hRequest, NULL, 0, 0)) {
		return false;
	}
	return true;
}

void HttpRequest::UpLoadFile(string filePath) {
	HINTERNET pServer, pSession;
	pSession = InternetOpen("seantest ", INTERNET_OPEN_TYPE_PRECONFIG, " ", " ",
			INTERNET_FLAG_ASYNC);
	pServer = InternetConnect(pSession, this->host.c_str(),
	/*INTERNET_INVALID_PORT_NUMBER*/this->port, NULL, NULL,
			INTERNET_SERVICE_HTTP, INTERNET_FLAG_NO_CACHE_WRITE, 0);

	INTERNET_BUFFERS BufferIn = { 0 };
	DWORD dwBytesRead;
	DWORD dwBytesWritten;
	BYTE pBuffer[1024];
	BOOL bRead, bRet;
	char res[1024];

	BufferIn.dwStructSize = sizeof(INTERNET_BUFFERS);
	BufferIn.lpcszHeader = "Content-Type: multipart/form-data; boundary=%s\r\n";

	TCHAR szAccept[] = "*/*";
	LPTSTR AcceptTypes[2] = { 0 };
	AcceptTypes[0] = szAccept;

	char strPort[6];
	sprintf(strPort, "%d", this->port);
	string para = this->GetParameters();
	if (para != "") {
		para = "?" + para;
	}
	string fullurl = "http://" + host + ":" + strPort + url + para;
	HINTERNET hRequest = HttpOpenRequest(pServer, "POST", fullurl.c_str(),
			"HTTP/1.1", NULL, (LPCTSTR*) AcceptTypes,
			INTERNET_FLAG_NO_CACHE_WRITE, 0);

	if (!hRequest) {
		sprintf(res, "Failed   to   open   request   handle:   %lu\n ",
				GetLastError());
		return;
	}

	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	BufferIn.dwBufferTotal = GetFileSize(hFile, NULL);

	if (!HttpSendRequestEx(hRequest, &BufferIn, NULL, HSR_INITIATE, 0)) {
		sprintf(res, "Error   on   HttpSendRequestEx   %lu\n ", GetLastError());
		return;
	}

	DWORD sum = 0;
	do {
		if (!(bRead = ReadFile(hFile, pBuffer, sizeof(pBuffer), &dwBytesRead,
				NULL))) {
			printf(res, "ReadFile   failed   on   buffer   %lu. ",
					GetLastError());
			break;
		}
		if (!(bRet = InternetWriteFile(hRequest, pBuffer, dwBytesRead,
				&dwBytesWritten))) {
			sprintf(res, "InternetWriteFile   failed   %lu ", GetLastError());
			break;
		}
		sum += dwBytesWritten;
	} while (dwBytesRead == sizeof(pBuffer));

	CloseHandle(hFile);

	if (!HttpEndRequest(hRequest, NULL, 0, 0)) {
		printf(res, "Error   on   HttpEndRequest   %lu   \n ", GetLastError());
		return;
	}
	return;
}

HttpRequest& HttpRequest::SetParameters(Properties& prop) {
	map<string, string>::iterator it;
	this->Clear();
	for (it = prop.Begin(); it != prop.End(); it++) {
		this->Put(it->first, it->second);
	}
	return *this;
}

void HttpRequest::Release() {
	delete this;
}

bool HttpRequest::PostRelease(int times) {
	bool ret = this->Post(times);
	delete this;
	return ret;
}

bool HttpRequest::GetRelease(int times) {
	bool ret = this->Get(times);
	delete this;
	return ret;
}

HttpRequest& HttpRequest::Create() {
	return *(new HttpRequest());
}

HttpRequest& HttpRequest::Create(RequestParameter &param) {
	return (new HttpRequest())->SetRequestParameter(param);
}
