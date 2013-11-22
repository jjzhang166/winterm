/*
 * DataBuffer.cpp
 *
 *  Created on: 2012-8-30
 *      Author: zhangbo
 */

#include "DataBuffer.h"
#include <memory.h>
#include <string>
using namespace std;

DataBuffer::DataBuffer() {
	// TODO Auto-generated constructor stub
}

DataBuffer::DataBuffer(unsigned long size) {
	this->size = size;
	this->buffer = new unsigned char[size];
	length = 0;
}

DataBuffer::~DataBuffer() {
	delete[] buffer;
}

bool DataBuffer::OnReceive(int size, void* data) {
	if (length + size <= this->size) {
		memcpy(buffer + length, data, size);
		length += size;
	} else {
		memcpy(buffer + length, data, this->size - length);
		length += this->size - length;
		if (!this->BufferProcess()) {
			return false;
		}
		data = ((char*) data) + this->size - length;
		length = size - (this->size - length);
		memcpy(buffer, data, length);
	}
	return true;
}

bool DataBuffer::BufferProcess() {
	return false;
}

void DataBuffer::Clear() {
	this->length = 0;
}

unsigned long DataBuffer::GetLength() {
	return this->length;
}

unsigned char* DataBuffer::GetBuffer() {
	return this->buffer;
}
