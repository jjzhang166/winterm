/*
 * comaccess.c
 *
 *  Created on: 2013年11月7日
 *      Author: zhangbo
 */

#include "comaccess.h"

#include "putty.h"

extern void *backhandle;
extern Backend *back;

void comapi_ex_send(char* data, int len) {
	back->send(backhandle, data, len);
}
