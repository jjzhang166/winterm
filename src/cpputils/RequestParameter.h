/*
 * RequestParameter.h
 *
 *  Created on: 2012-9-4
 *      Author: zhangbo
 */

#ifndef REQUESTPARAMETER_H_
#define REQUESTPARAMETER_H_

#include "Properties.h"

class RequestParameter: public Properties {
private:
	string host;
	int port;
	string url;
	unsigned long timeout;

public:
	RequestParameter();
	RequestParameter(const string& host, int port, const string& url);
	RequestParameter(const string& host, int port, const string& url,
			unsigned long timeout);
	RequestParameter(const string& host, int port, const string& url,
			const string& method);

	virtual ~RequestParameter();

	void SetHost(const string& host);
	void SetPort(int port);
	void SetUrl(const string& url);
	void SetTimeout(unsigned long timeout);

	string GetHost();
	int GetPort();
	string GetUrl();
	unsigned long GetTimeout();
};

#endif /* REQUESTPARAMETER_H_ */
