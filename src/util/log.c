/*
 * log.c
 *
 *  Created on: 2013-4-12
 *      Author: zhangbo
 */

#include "log.h"

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "types.h"
#include "des.h"
static FILE* plogfile;
static CRITICAL_SECTION critsec;
static int init = 0;
static int star = 0;
static int encr = TRUE;
static int debug = TRUE;
static unsigned char key[] = "!@#$%^&*";

void log_open() {
	InitializeCriticalSection(&critsec);
	char tm[1024];
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	sprintf(tm, "%04d%02d%02d%02d%02d%02d%03d.02.log", sys.wYear, sys.wMonth,
			sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);
	plogfile = fopen(tm, "wb");
}

void log_set_enable(int enable) {
	debug = enable;
}

void log_set_encrypt(int enable) {
	encr = enable;
}

void log_close() {
	fclose(plogfile);
	DeleteCriticalSection(&critsec);
}

void log_star() {
	star = 1;
}

void log_stop() {
	star = 0;
}

void log_write_str(const char* type, const char * data) {
	log_write(type, data, strlen(data));
}

void log_write_err(const char* type, const char* data, unsigned long err) {
	char str[1024];
	sprintf(str, "%s(%ld)", data, err);
	log_write_str(type, str);
}

void log_write(const char* type, const void* data, unsigned long len) {
	int i;
	char log[1024];
	SYSTEMTIME sys;
	char tm[128];

	//临时加入的，客户不需要修改为调试，系统默认为调试状态。
	if (debug) {
		if (init == 0) {
			init++;
			log_open();
			log_star();
		}
		EnterCriticalSection(&critsec);

		GetLocalTime(&sys);
		sprintf(tm, "%04d-%02d-%02d %02d:%02d:%02d.%03d", sys.wYear, sys.wMonth,
				sys.wDay, sys.wHour, sys.wMinute, sys.wSecond,
				sys.wMilliseconds);

		sprintf(log, "%s %s %05ld\r\n", type, tm, len);
		fwrite(log, strlen(log), 1, plogfile);

		if (!encr) {
			for (i = 0; i < len; i++) {
				sprintf(log, "%02X ", ((unsigned char*) data)[i]);
				fwrite(log, strlen(log), 1, plogfile);
			}
			fwrite("\r\n(", 3, 1, plogfile);
			fwrite(data, len, 1, plogfile);
			fwrite(")\r\n\r\n", 5, 1, plogfile);
		} else {
			unsigned char enc[8];
			int ptr = 0;
			while (ptr + 8 <= len) {
				des(((unsigned char*)data) + ptr, key, enc, DES_ENCRYPT);
				fwrite(enc, 8, 1, plogfile);
				ptr += 8;
			}
			if (ptr < len) {
				unsigned char dat[8];
				memset(dat, ' ', 8);
				memcpy(dat, ((unsigned char*)data)+ptr, len-ptr);
				des(dat, key, enc, DES_ENCRYPT);
				fwrite(enc, 8, 1, plogfile);
			}
			fwrite("\r\n\r\n", 4, 1, plogfile);
		}

		fflush(plogfile);
		LeaveCriticalSection(&critsec);
	}
}
