/*
 * Configure.h
 *
 *  Created on: 2012-8-30
 *      Author: zhangbo
 */

#ifndef CONFIGURE_H_
#define CONFIGURE_H_

#include "Properties.h"
#include "HttpRequest.h"

class Configure: public Properties {
private:
	static Configure* configure;
	string file;

public:
	Configure();
	virtual ~Configure();

	void SetFile(const string& file);

	string GetFile();

	static void SetDefault(const string& file);

	static Configure* Default();

	HttpRequest& GetHttpRequest(const string& pre);
};

#endif /* CONFIGURE_H_ */
