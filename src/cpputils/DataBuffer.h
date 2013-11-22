/*
 * DataBuffer.h
 *
 *  Created on: 2012-8-30
 *      Author: zhangbo
 */

#ifndef DATABUFFER_H_
#define DATABUFFER_H_

#include "DataReceiver.h"

class DataBuffer: public DataReceiver {
private:
	unsigned long size;

protected:
	unsigned char* buffer;
	unsigned long length;

public:
	DataBuffer();

	DataBuffer(unsigned long size);

	virtual ~DataBuffer();

	void SetSize(unsigned long size);

	unsigned long GetLength();

	unsigned char* GetBuffer();

	bool OnReceive(int size, void* data);

	void Clear();

	virtual bool BufferProcess();

};

#endif /* DATABUFFER_H_ */
