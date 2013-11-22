/*
 * print.h
 *
 *  Created on: 2012-11-7
 *      Author: zhangbo
 */

#ifndef PRINT_H_
#define PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

void* lptopen(unsigned long port);
unsigned long lptwrite(void* lpt, const void* data, int len);
void lptclose(void* lpt);
int lptclear();

#ifdef __cplusplus
}
#endif

#endif /* PRINT_H_ */
