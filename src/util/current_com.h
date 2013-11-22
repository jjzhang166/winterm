/*
 * current_com.h
 *
 *  Created on: 2012-9-20
 *      Author: zhangbo
 */

#ifndef CURRENT_COM_H_
#define CURRENT_COM_H_

#include "windows/com.h"

#ifdef  __cplusplus
extern "C" {
#endif

int current_com_init();

int current_com_uninit();

int current_com_setup(COMPARAM* comPtr);

int current_com_open();

int current_com_close();

int current_com_write(unsigned char *data, long len);

unsigned long current_com_read(unsigned char *data, unsigned long len);

#ifdef  __cplusplus
}
#endif

#endif /* CURRENT_COM_H_ */
