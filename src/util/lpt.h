/*
 * lpt.h
 *
 *  Created on: 2012-11-7
 *      Author: xia
 */

#ifndef LPT_H_
#define LPT_H_

void* prtopen(const char* lpt);
unsigned long prtwrite(void* lpt, void* data, int len);
void prtclose(void* lpt);

#endif /* LPT_H_ */
