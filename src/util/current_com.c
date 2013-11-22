/*
 * current_com.cpp
 *
 *  Created on: 2012-9-20
 *      Author: zhangbo
 */

#include "current_com.h"

#include <windows.h>

static CRITICAL_SECTION critsec;
static HANDLE com = NULL;
static COMPARAM comParam;

int current_com_init() {
	InitializeCriticalSection(&critsec);
	return 0;
}

int current_com_uninit() {
	DeleteCriticalSection(&critsec);
	return 0;
}

int current_com_setup(LPCOMPARAM comPtr) {
	EnterCriticalSection(&critsec);
	memcpy(&comParam, comPtr, sizeof(COMPARAM));
	LeaveCriticalSection(&critsec);
	return 0;
}

int current_com_open() {
	unsigned long ret = 0;
	EnterCriticalSection(&critsec);
	if (com != NULL) {
		com_close(com);
		com = NULL;
	}
	com = com_open(&comParam);
	if (com == NULL) {
		ret = -1;
	}
	LeaveCriticalSection(&critsec);
	return ret;
}

int current_com_close() {
	unsigned long ret = -1;
	EnterCriticalSection(&critsec);
	if (com != NULL) {
		com_close(com);
		com = NULL;
		ret = 0;
	}
	LeaveCriticalSection(&critsec);
	return ret;
}

int current_com_write(unsigned char *writeStr, long strLen) {
	unsigned long ret = -1;
	EnterCriticalSection(&critsec);
	if (com != NULL) {
		ret = com_write(com, writeStr, strLen);
	}
	LeaveCriticalSection(&critsec);
	return ret;
}

unsigned long current_com_read(unsigned char *readStr, unsigned long maxStrLen) {
	unsigned long ret = -1;
	EnterCriticalSection(&critsec);
	if (com != NULL) {
		ret = com_read(com, readStr, maxStrLen);
	}
	LeaveCriticalSection(&critsec);
	return ret;
}
