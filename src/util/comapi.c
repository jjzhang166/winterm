/*
 * Extends.cpp
 *
 *  Created on: 2012-9-26
 *      Author: zhangbo
 */

#include "comapi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <dos.h>
#include <io.h>
#include <conio.h>
#include "types.h"
#include "current_com.h"
#include "log.h"

static int setup = 0;
int com_enable = 0;
extern char comchannel;
static int output = 0;
static int direct_mode = 1;
static int line_mode = 0;
static int running = 0;

static void comapi_read_proc_start();

extern void comapi_ex_send(char* data, int len);

int comapi_setup(unsigned char com, int baudRate, int parity, int byteSize, int stopBits) {
	COMPARAM p;
	p.bEvtChar = 0x0d;
	p.fBinary = 1;

	p.bComId = com;

	switch (baudRate) {
	case 0:
		p.dwBaudRate = 9600;
		break;
	case 1:
		p.dwBaudRate = 4800;
		break;
	case 2:
		p.dwBaudRate = 2400;
		break;
	case 3:
		p.dwBaudRate = 1200;
		break;
	case 4:
		p.dwBaudRate = 900;
		break;
	case 5:
		p.dwBaudRate = 600;
		break;
	case 6:
		p.dwBaudRate = 300;
		break;
	case 7:
		p.dwBaudRate = 150;
		break;
	case 8:
		p.dwBaudRate = 75;
		break;
	case 9:
		p.dwBaudRate = 800;
		break;
	default:
		p.dwBaudRate = 9600;
		break;
	}

	switch (parity) {
	case 0: //none
		p.bParity = 0;
		break;
	case 1: //1
		p.bParity = 1;
		break;
	case 2:
		p.bParity = 2;
		break;
	default:
		p.bParity = 0;
		break;
	}

	switch (byteSize) {
	case 0:
		p.bByteSize = 8;
		break;
	case 1:
		p.bByteSize = 7;
		break;
	default:
		p.bByteSize = 8;
		break;
	}

	switch (stopBits) {
	case 0:
		p.bStopBits = 0; //1
		break;
	case 1:
		p.bStopBits = 2; //2
		break;
	default:
		p.bStopBits = 1; //1.5
		break;
	}

	current_com_setup(&p);
	setup = 1;
	if (com_enable && setup) {
		current_com_open();
		comapi_read_proc_start();
	}
	return TRUE;
}

#define COM_READ_BUFFER_SIZE 1024

static void comapi_send(unsigned char* data, unsigned long len) {
	if (!direct_mode) {
		int i = 0, j = 0;
		char* pout = malloc(len * 3);
		for (i = 0; i < len; i++) {
			pout[j++] = 0x80;
			pout[j++] = data[i];
			pout[j++] = 0x81;
		}
		comapi_ex_send(pout, len * 3);
		free(pout);
	} else {
		comapi_ex_send((char*) data, len);
	}
}

static unsigned char inBuffer[COM_READ_BUFFER_SIZE];
static unsigned long inLen = 0;
static unsigned char tempBuffer[COM_READ_BUFFER_SIZE * 10];
static long tempLen = 0;
static unsigned long WINAPI comapi_read_thread(void* device) {
	log_write_str(COM_INFO, "com_read_thread start");
	while (running) {
		Sleep(5);
		if (!com_enable) {
			continue;
		}
		inLen = current_com_read(inBuffer, COM_READ_BUFFER_SIZE);
		if (inLen == 0) {
			continue;
		}
		if (line_mode) {
			int last = 0, i = 0;

			for (i = 0; i < inLen; i++) {
				if (inBuffer[i] == 0x0D) {
					memcpy(tempBuffer + tempLen, inBuffer + last, i + 1 - last);
					tempLen += i + 1 - last;
					last = i + 1;
					comapi_send(tempBuffer, tempLen);
					tempLen = 0;
				}
			}
			memcpy(tempBuffer + tempLen, inBuffer + last, i - last);
			tempLen += i - last;
		} else {
			comapi_send(inBuffer, inLen);
		}
	}
	log_write_str(COM_INFO, "com_read_thread ended");

	return 0;
}

static void comapi_read_proc_start() {
	if (running == 0) {
		CreateThread(NULL, 0, comapi_read_thread, NULL, 0, 0);
		running = 1;
	}
}

static void comapi_read_proc_stop() {
	running = 0;
	Sleep(10);
}

int comapi_set_enable(int state) {
	com_enable = state;
	if (com_enable && setup) {
		if (current_com_open() == -1) {
			com_enable = 0;
			return FALSE;
		}
		comapi_read_proc_start();
	} else {
		comapi_read_proc_stop();
		current_com_close();
		comchannel = 0;
		setup = 0;
	}
	return TRUE;
}

void comapi_set_output(int state) {
	output = state;
	if (state) {
		log_write_str(COM_INFO, "com out 1");
	} else {
		log_write_str(COM_INFO, "com out 0");
	}
}

int comapi_writing_enable() {
	return output && com_enable;
}

void comapi_set_line_mode(int state) {
	line_mode = state;
	if (state) {
		log_write_str(COM_INFO, "line mode 1");
	} else {
		log_write_str(COM_INFO, "line mode 0");
	}
}

void comapi_set_direct_mode(int state) {
	direct_mode = state;
	if (state) {
		log_write_str(COM_INFO, "direct mode 1");
	} else {
		log_write_str(COM_INFO, "direct mode 0");
	}
}

int comapi_receive(unsigned char* data, unsigned long len) {
	return current_com_write(data, len);
}
