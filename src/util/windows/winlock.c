/*
 * winlock.c
 *
 *  Created on: 2013年11月11日
 *      Author: zhangbo
 */

#include "winlock.h"

#include <malloc.h>
#include <windows.h>

void* create_lock() {
	CRITICAL_SECTION *critsec = (CRITICAL_SECTION *) malloc(
			sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(critsec);
	return critsec;
}

void lock(void* lk) {
	EnterCriticalSection(lk);
}

void unlock(void* lk) {
	LeaveCriticalSection(lk);
}

void release_lock(void* lk) {
	DeleteCriticalSection(lk);
	free(lk);
}
