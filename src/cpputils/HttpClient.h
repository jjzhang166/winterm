/*
 * HttpClient.h
 *
 *  Created on: 2012-11-27
 *      Author: zhangbo
 */

#ifndef HTTPCLIENT_H_
#define HTTPCLIENT_H_

#include "DataReceiver.h"
#include "Properties.h"
#include <string>
using namespace std;

class HttpClient: public Properties {
private:
	DataReceiver* listener;
	string host;
	string url;
	int port;
	unsigned long timeout;
	string file;
	string id;

public:
	HttpClient();
	HttpClient(const string& host, int port, const string& url);
	virtual ~HttpClient();

	void SetHost(const string& host);
	string GetHost();

	void SetFile(const string& file);
	string GetFile();

	void SetId(const string& id);
	string GetId();

	void SetPort(int port);
	int GetPort();

	void SetTimeout(unsigned long timeout);
	unsigned long GetTimeout();

	void SetUrl(const string& url);
	string GetUrl();

	HttpClient& SetListener(DataReceiver* listener);

	int Post();
};

#endif /* HTTPCLIENT_H_ */
