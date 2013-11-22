/*
 * InfoDownloader.h
 *
 *  Created on: 2012-8-30
 *      Author: zhangbo
 */

#ifndef INFODOWNLOADER_H_
#define INFODOWNLOADER_H_

#include "DataBuffer.h"
#include "Properties.h"

class InfoDownloader: public DataBuffer, public Properties {
public:
	InfoDownloader(unsigned long size);
	virtual ~InfoDownloader();

	bool Process();
};

#endif /* INFODOWNLOADER_H_ */
