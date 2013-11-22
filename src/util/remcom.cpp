/*
 * remcom.cpp
 *
 *  Created on: 2013-1-25
 *      Author: zhangbo
 */

#include <stdio.h>
#include "remcom.h"
#include "remote.h"
#include "../putty.h"
static unsigned int handle = 0;

int remote_com_setup(LPCOMPARAM comPtr) {
	if (handle == 0) {
		handle = remote_open("127.0.0.1", 5678);
		char cmd[6];
		sprintf(cmd, "COM%2d", cfg.screennum); //屏号
		remote_send(handle, cmd, 5);
	}
	remote_send(handle, "SET", 3);
	remote_send(handle, comPtr, sizeof(COMPARAM));
	return 0;
}

int remote_com_open() {
	remote_send(handle, "OPE", 3);
	return 0;
}

int remote_com_close() {
	remote_send(handle, "CLO", 3);
	return 0;
}

int remote_com_write(unsigned char *writeStr, int strLen) {
	remote_send(handle, "WRI", 3);
	remote_send(handle, &strLen, sizeof(int));
	remote_send(handle, writeStr, strLen);
	return 0;
}

unsigned long remote_com_read(unsigned char *readStr, unsigned long maxStrLen) {
	unsigned long size = 0;
	remote_send(handle, "REA", 3);
	remote_recv(handle, &size, sizeof(size));
	if (size > 0) {
		remote_recv(handle, readStr, size);
	}
	return size;
}
