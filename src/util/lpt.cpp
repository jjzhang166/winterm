/*
 * lpt.c
 *
 *  Created on: 2012-11-7
 *      Author: xia
 */

#include "lpt.h"
#include <windows.h>
#include "windows/WinIo.h"

void* prtopen(const char* name) {
	char str[] = "C:\\WinIo.sys";
	if (InstallWinIoDriver(str, FALSE)) {
		if (InitializeWinIo()) {
			return 0;
		}
	}
	return 0;
}

unsigned long prtwrite(void* lpt, void* data, int len) {
	unsigned long chr;
	for (int i = 0; i < len; i++) {
		if (SetPortVal(0x378, *(((char*) data) + i), 1)) {
			chr = 12;
		}
		GetPortVal(0x379, &chr, 1);
	}
	return 0;
}

void prtclose(void* lpt) {
	ShutdownWinIo();
	RemoveWinIoDriver();
}
