/*
 * InfoDownloader.cpp
 *
 *  Created on: 2012-8-30
 *      Author: zhangbo
 */

#include "InfoDownloader.h"
#include "Util.h"

InfoDownloader::InfoDownloader(unsigned long size) :
		DataBuffer(size) {
	// TODO Auto-generated constructor stub

}

InfoDownloader::~InfoDownloader() {
	// TODO Auto-generated destructor stub
}

bool InfoDownloader::Process() {
	if (this->GetLength() < 38) {
		return false;
	}

	unsigned char* md5 = buffer + this->GetLength() - 32;
	unsigned char newmd5[16];
	Util::MD5(buffer, this->GetLength() - 33, newmd5);
	string temp = Util::Print(newmd5, 16);
	if (0 == memcmp(md5, temp.c_str(), 32)) {
		return this->Load(this->GetBuffer(), this->GetLength());
	}
	return false;
}

