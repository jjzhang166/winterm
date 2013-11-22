/*
 * remcom.h
 *
 *  Created on: 2013-1-25
 *      Author: zhangbo
 */

#ifndef REMCOM_H_
#define REMCOM_H_

#include "windows/com.h"

#ifdef  __cplusplus
extern "C" {
#endif

int remote_com_setup(COMPARAM* comPtr);

int remote_com_open();

int remote_com_close();

int remote_com_write(unsigned char *data, int len);

unsigned long remote_com_read(unsigned char *data, unsigned long len);

#ifdef  __cplusplus
}
#endif
#endif /* REMCOM_H_ */
