/*
 * NetInformation.h
 *
 *  Created on: 2012-9-3
 *      Author: zhangbo
 */

#ifndef NETINFORMATION_H_
#define NETINFORMATION_H_

#include "InfoDownloader.h"
#include "HttpRequest.h"

class NetInformation: public InfoDownloader {
private:
	HttpRequest request;
	Properties properties;

public:
	NetInformation(const string& host, int port, const string& url,
			const unsigned long size = 4096);
	virtual ~NetInformation();

	void SetProperties(const Properties& prop);

	bool Update();
};

#endif /* NETINFORMATION_H_ */
