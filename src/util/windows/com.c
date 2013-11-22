/*
 * comapi.cpp
 *
 *  Created on: 2012-9-20
 *      Author: zhangbo
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "com.h"
#include "../types.h"
#include "../log.h"

void* com_open(LPCOMPARAM param) {
	char com[16];
	DCB dcb;
	HANDLE hComm;
	COMMTIMEOUTS timeouts;

	memset(com, 0x00, 16);
	sprintf(com, "\\\\.\\COM%d", param->bComId);
	hComm = CreateFile(com, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, 0);
	if (hComm == INVALID_HANDLE_VALUE) {
		log_write_err(COM_OPEN, "ERROR CreateFile", GetLastError());
		CloseHandle(hComm);
		return NULL;
	}

	if (!SetupComm(hComm, 1024*512, 1024*512)){ //输入缓冲区和输出缓冲区的大小都是2048{
		log_write_err(COM_OPEN, "ERROR SetupComm", GetLastError());
	}

	if (!GetCommState(hComm, &dcb)) {
		log_write_err(COM_OPEN, "ERROR GetCommState", GetLastError());
		CloseHandle(hComm);
		return NULL;
	}

	dcb.BaudRate = param->dwBaudRate;
	dcb.Parity = param->bParity;
	if (!dcb.Parity) {
		dcb.fParity = 0;
	}
	dcb.StopBits = param->bStopBits;
	dcb.ByteSize = param->bByteSize;
	dcb.fBinary = param->fBinary;
	dcb.EvtChar = param->bEvtChar;

	if (!SetCommState(hComm, &dcb)) {
		log_write_err(COM_OPEN, "ERROR SetCommState", GetLastError());
		CloseHandle(hComm);
		return NULL;
	}

	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	if (!SetCommTimeouts(hComm, &timeouts)) {
		log_write_err(COM_OPEN, "ERROR SetCommTimeouts", GetLastError());
		CloseHandle(hComm);
		return NULL;
	}

//	SetCommMask(hComm, EV_RXCHAR);
	PurgeComm(hComm,
			PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	char data[1024];
	sprintf(data, "COM%d", param->bComId);
	log_write_str(COM_OPEN, data);

	return hComm;
}

unsigned long com_read(void* com, unsigned char* buffer, unsigned long bufLen) {
	BOOL bResult = TRUE;
	DWORD dwError = 0;
	DWORD dwReadLen = 0;
	COMSTAT comstat;
	DWORD dwToReadLen = 0;

	bResult = ClearCommError(com, &dwError, &comstat);
	if(!bResult) {
		log_write_err(COM_RECV, "ERROR ClearCommError", GetLastError());
	}
	if (comstat.cbInQue == 0) {
		return 0;
	}

	dwToReadLen = bufLen;
	if (comstat.cbInQue < bufLen) {
		dwToReadLen = comstat.cbInQue;
	}

	bResult = ReadFile(com, buffer, dwToReadLen, &dwReadLen, NULL);
	if (!bResult) {
		//读取失败
		log_write_err(COM_RECV, "ERROR ReadFile", GetLastError());
		PurgeComm(com, PURGE_RXABORT | PURGE_RXCLEAR);
		return 0;
	}

	if (dwReadLen > 0) {
		log_write(COM_RECV, buffer, dwReadLen);
	}
	return dwReadLen;
}

unsigned long com_write(void* com, const unsigned char* data, unsigned long len) {
	BOOL bResult = TRUE;
	DWORD dwWriteLen = 0;

	bResult = WriteFile(com, data, len, &dwWriteLen, NULL);
	if (!bResult) {
		//写入失败
		log_write_err(COM_SEND, "ERROR WriteFile", GetLastError());
		PurgeComm(com, PURGE_TXABORT | PURGE_TXCLEAR);
		return 0;
	}

	log_write(COM_SEND, data, dwWriteLen);

	return dwWriteLen;
}

int com_close(void* com) {
	BOOL ret = CloseHandle(com);
	if (ret) {
		log_write_str(COM_CLOS, "SUCCESS");
	} else {
		log_write_err(COM_CLOS, "ERROR", GetLastError());
	}
	return 0;
}

void mainmain() {
	unsigned char cs1[] = { 0xAA, 0xE5, 0xAA, 0xE5, 0xAA, 0x02, 0x43, 0x30,
			0x30, 0x30, 0x30, 0x03 };
	unsigned char cs2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00 };
	COMPARAM parm;
	parm.bByteSize = 8;
	parm.bComId = 1;
	parm.bParity = 0;
	parm.bStopBits = 0;
	parm.dwBaudRate = 9600;
	parm.fBinary = 1;
	void* hComm = com_open(&parm);
	if (hComm == NULL) {
		MessageBox(NULL, "not exist", "ok",
				MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1);
		return;
	}
	unsigned long len = com_write(hComm, cs1, 12);
	if (len == 0) {
		MessageBox(NULL, "write error", "ok",
				MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1);
	}

	int i = 0;
	while (i++ < 10) {
		if (com_read(hComm, cs2, 7) > 0) {
			break;
		}
	}

	com_close(hComm);

//	unsigned char cs1[] = { 0xAA, 0xE5, 0xAA, 0xE5, 0xAA, 0x02, 0x43, 0x30,
//			0x30, 0x30, 0x30, 0x03 };
//	unsigned char cs2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//			0x00, 0x00, 0x00, 0x00 };
//	com_read_proc_start();
//	com_setup('Y',0,0,0,0);
//	com_set_line_mode(0);
//	com_set_direct_mode(1);
//	com_set_enable(1);
//	com_set_output(1);
//	com_receive(cs1,12);//com_write(hComm, cs1, 12);
//	Sleep(1000);
//	com_read_proc_stop();
}
