#pragma once

#include "DataReceiver.h"
#include "Properties.h"
#include "RequestParameter.h"
#include "Logger.h"
#include <string>

using namespace std;

class HttpRequest: public Properties {
private:
	DataReceiver* listener;
	string host;
	string url;
	string file;
	int port;
	unsigned long timeout;
	static Logger* log;
	void Log(const string& url);

public:
	HttpRequest(void);
	HttpRequest(const string& host, int port, const string& url);
	~HttpRequest(void);

	HttpRequest& SetRequestParameter(RequestParameter& param);

	void SetHost(const string& host);
	string GetHost();

	void SetPort(int port);
	int GetPort();

	void SetTimeout(unsigned long timeout);
	unsigned long GetTimeout();

	void SetUrl(const string& url);
	string GetUrl();

	HttpRequest& Append(const string& name, const string& value);

	bool Post(int times);

	bool Get(int times);

	bool Process(int times, string method);

	bool Post();

	bool Get();

	void UpLoadFile(string filePath);

	HttpRequest& SetListener(DataReceiver* listener);

	string GetParameters();

	HttpRequest& SetParameters(Properties& prop);

	bool PostRelease(int times = 1);

	bool GetRelease(int times = 1);

	void Release();

	static HttpRequest& Create();

	static HttpRequest& Create(RequestParameter &param);
};

