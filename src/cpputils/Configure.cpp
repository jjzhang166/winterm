/*
 * Configure.cpp
 *
 *  Created on: 2012-8-30
 *      Author: zhangbo
 */

#include "Configure.h"

Configure* Configure::configure = NULL;

Configure::Configure() {
	// TODO Auto-generated constructor stub

}

Configure::~Configure() {
	// TODO Auto-generated destructor stub
}

void Configure::SetFile(const string& file) {
	this->file = file;
	this->SafeLoad(file);
}

string Configure::GetFile() {
	return this->file;
}

void Configure::SetDefault(const string& file) {
	if (Configure::configure == NULL) {
		Configure::configure = new Configure();
	}
	Configure::configure->SetFile(file);
}

Configure* Configure::Default() {
	return Configure::configure;
}

HttpRequest& Configure::GetHttpRequest(const string& pre) {
	return *(new HttpRequest(this->GetString(pre + ".host"),
			this->GetInteger(pre + ".port"), this->GetString(pre + ".url")));
}
