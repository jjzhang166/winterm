/*
 * NetInformation.cpp
 *
 *  Created on: 2012-9-3
 *      Author: zhangbo
 */

#include "NetInformation.h"
#include <string>
using namespace std;

NetInformation::NetInformation(const string& host, int port, const string& url,
		const unsigned long size) :
		InfoDownloader(size) {
	request.SetHost(host);
	request.SetPort(port);
	request.SetUrl(url);
	request.SetListener(this);
}

NetInformation::~NetInformation() {
}

void NetInformation::SetProperties(const Properties& prop) {
	this->properties = prop;
	this->request.SetParameters(this->properties);
}

bool NetInformation::Update() {
	this->length = 0;
	bool ret = request.Get();
	if (ret) {
		ret = this->Process();
	}
	return ret;
}
