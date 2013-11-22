/*
 * print.c
 *
 *  Created on: 2012-11-7
 *      Author: zhangbo
 */

#include "print.h"
#include <windows.h>
#include "../types.h"
#include "WinIo.h"
#include <time.h>

//#define PRT_OLD

typedef struct _LPT {
	unsigned long data;
	unsigned long stat;
	unsigned long ctrl;
} LPT;

static LPT* lpt = NULL;

#ifdef PRT_OLD

void* lptopen(unsigned long port) {
	if (InitializeWinIo()) {
		LPT* llpt = (LPT*) malloc(sizeof(LPT));
		llpt->data = port;
		llpt->stat = port + 1;
		llpt->ctrl = port + 2;
		lpt = llpt;
		return llpt;
	}
	return NULL;
}

void lptclose(void* lpt) {
	ShutdownWinIo();
	free(lpt);
}

unsigned long lptwrite(void* vlpt, const void* data, int len) {
	DWORD dwPortVal = 0;
	int i = 0;
	DWORD cmdBy[8];
	int Done = 0;
	byte stateNum = 0;
	LPT* lpt = (LPT*) vlpt;

	DWORD readCMD = 0;
	//取时间差
	time_t tbegin, tend;
	long timeCha = 0;

	for (i = 0; i < len; i++) {
		Done = 0;
		while (!Done) {
			tend = time(NULL); //结束时间

			timeCha = tend - tbegin;//时间差;

			if (timeCha >= 20) {
				lptclose(lpt);
				lpt = lptopen(0x378);
				tbegin = tend;
			}
			//查询并口状态，并口可写入时，写入数据
			GetPortVal(lpt->stat, &dwPortVal, 1);
			dwPortVal = dwPortVal & 0x0FF;//取低8位
			stateNum = (byte) dwPortVal;

			//判断S7位为低电平说明打印机不正常或忙状态,异常为0
			if ((stateNum & 0x80) == 0x00) {
				Done = 0;
			}
			//判断S5位为低电平说明缺纸状态,异常为1
			else if ((stateNum & 0x20) == 0x20) {
				Done = 0;
			}
			//判断S4位为低电平说明联机状态,异常为0
			else if ((stateNum & 0x10) == 0x00) {
				Done = 0;
			}
			//判断S3位为低电平说明打印机未知错误,异常为0
			else if ((stateNum & 0x08) == 0x00) {
				Done = 1;
				return -1;
			} else {
				Done = 1;
			}
		}

		SetPortVal(lpt->data, ((char*) data)[i], 1);

		GetPortVal(lpt->ctrl, &readCMD, 1);
		cmdBy[1] = readCMD | 0x09;
		SetPortVal(lpt->ctrl, cmdBy[1], 1);

		GetPortVal(lpt->ctrl, &readCMD, 1);
		cmdBy[2] = readCMD & 0xFE;
		SetPortVal(lpt->ctrl, cmdBy[2], 1);
	}
	return 0;
}

int lptclear() {
	DWORD dwMemVal;
	DWORD cmdBy[8];

	GetPortVal(lpt->ctrl, &dwMemVal, 1);
	cmdBy[2] = 0x04;
	SetPortVal(lpt->ctrl, cmdBy[2], 1);
	return 0;
}

#else

void* lptopen(unsigned long port) {
	char drv[] = "./WinIo.sys";
	if (InstallWinIoDriver(drv, FALSE)) {
		if (InitializeWinIo()) {
			DWORD readCMD = 0;
			LPT* pLpt = (LPT*) malloc(sizeof(LPT));
			pLpt->data = port;
			pLpt->stat = port + 1;
			pLpt->ctrl = port + 2;
			lpt = pLpt;
			GetPortVal(pLpt->ctrl, &readCMD, 1);
			readCMD = readCMD | 0x08;
			SetPortVal(pLpt->ctrl, readCMD, 1);
			return pLpt;
		}
	}
	return 0;
}

static BOOL wait(LPT* plpt) {
	byte status = 0;
	DWORD dwPortVal = 0;
	BOOL done = FALSE;
	int count = 0;
	while ((!done) && (count < 10)) {
		count++;
		GetPortVal(plpt->stat, &dwPortVal, 1);
		dwPortVal = dwPortVal & 0x0FF;
		status = (byte) dwPortVal;

//		if ((status & 0x80) == 0x00) {
//			//判断S7位为低电平说明打印机不正常或忙状态,异常为0
//			done = FALSE;
//		} else if ((status & 0x20) == 0x20) {
//			//判断S5位为低电平说明缺纸状态,异常为1
//			done = FALSE;
//		} else if ((status & 0x10) == 0x00) {
//			//当打印机处于无效状态时，该信号就会变成低电平
//			done = FALSE;
//		} else if ((status & 0x08) == 0x00) {
//			//判断S3位为低电平说明打印机未知错误,异常为0
//			done = FALSE;
//		} else {
//			done = TRUE;
//		}

		if ((status & 0x98) != 0x98 || (status & 0x20)) {
			done = FALSE;
		} else {
			done = TRUE;
		}
	}
	return done;
}

unsigned long lptwrite(void* lpt, const void* data, int len) {
	LPT* plpt = (LPT*) lpt;
	DWORD readCMD = 0;
	int i = 0;

	GetPortVal(plpt->ctrl, &readCMD, 1);
	for (i = 0; i < len; i++) {

		SetPortVal(plpt->data, *(((char*) data) + i), 1);

		readCMD = readCMD | 0x09;
		SetPortVal(plpt->ctrl, readCMD, 1);

		readCMD = readCMD & 0xFE;
		SetPortVal(plpt->ctrl, readCMD, 1);

		while (wait(plpt) == FALSE) {
			Sleep(1);
		}
	}

	return 0;

}

void lptclose(void* plpt) {
	ShutdownWinIo();
	free(plpt);
}

#endif
