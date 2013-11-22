/*
 * RequestParameter.cpp
 *
 *  Created on: 2012-9-4
 *      Author: zhangbo
 */

#include "RequestParameter.h"

RequestParameter::RequestParameter() {
	this->timeout = 20 * 1000;
}

RequestParameter::RequestParameter(const string& host, int port,
		const string& url) {
	this->host = host;
	this->port = port;
	this->url = url;
	this->timeout = 20 * 1000;
}

RequestParameter::RequestParameter(const string& host, int port,
		const string& url, unsigned long timeout) {
	this->host = host;
	this->port = port;
	this->url = url;
	this->timeout = timeout;
}

RequestParameter::RequestParameter(const string& host, int port,
		const string& url, const string& method) {
	this->host = host;
	this->port = port;
	this->url = url;
	this->timeout = 20 * 1000;
	this->Put("method", method);
}

RequestParameter::~RequestParameter() {
	// TODO Auto-generated destructor stub
}

void RequestParameter::SetHost(const string& host) {
	this->host = host;
}

void RequestParameter::SetPort(int port) {
	this->port = port;
}

void RequestParameter::SetUrl(const string& url) {
	this->url = url;
}

string RequestParameter::GetHost() {
	return this->host;
}

int RequestParameter::GetPort() {
	return this->port;
}

string RequestParameter::GetUrl() {
	return this->url;
}

void RequestParameter::SetTimeout(unsigned long timeout) {
	this->timeout = timeout;
}

unsigned long RequestParameter::GetTimeout() {
	return this->timeout;
}
