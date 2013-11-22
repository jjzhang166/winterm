/*
 * prtsvc.c
 *
 *  Created on: 2013-1-22
 *      Author: zhangbo
 */

#include <stdio.h>
#include "types.h"
#include "prtsvc.h"
#include "remote.h"
#include <winsock2.h>

unsigned int prt_open(char* ip, int port) {
	unsigned int handle = remote_open(ip, port);
//	char cmd[6];
//	sprintf(cmd, "PRT%2d", 10);//屏号
//	remote_send(handle, cmd, 5);
	return handle;
}

unsigned long prt_write(unsigned int sClient, const void* data, int len) {
	do {
		if (remote_send(sClient, (const char*) data, len) == 0) {
			break;
		}
	} while (FALSE);
	return 0;
}

void prt_close(unsigned int sClient) {
	remote_close(sClient);
}
