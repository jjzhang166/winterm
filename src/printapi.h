/*
 * printapi.h
 *
 *  Created on: 2013-9-11
 *      Author: zhangbo
 */

#ifndef PRINTAPI_H_
#define PRINTAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

//0为驱动打印；1串口打印；2直接打印；3文件打印；4共享打印；
#define PRINT_DRV 0
#define PRINT_COM 1
#define PRINT_LPT 2
#define PRINT_FIL 3
#define PRINT_SHR 4

void* print_new(char *name, char type, char whole);

void print_open(void* printer);
void print_data(void* printer, void* data, int len);
void print_close(void* printer);

void print_file(void* printer, char* file);

void print_del(void* printer);

#ifdef __cplusplus
}
#endif

#endif /* PRINTAPI_H_ */
