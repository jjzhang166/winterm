/*
 * comapi.h
 *
 *  Created on: 2012-9-26
 *      Author: zhangbo
 */

#ifndef COMAPI_H_
#define COMAPI_H_
#ifdef __cplusplus
extern "C" {
#endif

int comapi_setup(unsigned char com, int baudRate, int parity, int byteSize, int stopBits);

/**
 * 是否当前接收到的所有数据都写向串口（并且串口已经打开）
 */
int comapi_writing_enable();

int comapi_set_enable(int state);
void comapi_set_output(int state);
void comapi_set_line_mode(int state);
void comapi_set_direct_mode(int state);
int comapi_receive(unsigned char* data, unsigned long len);

#ifdef __cplusplus
}
#endif

#endif /* EXTENDS_H_ */
