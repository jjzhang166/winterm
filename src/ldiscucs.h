/*
 * ldiscucs.h
 *
 *  Created on: 2012-9-11
 *      Author: zhangbo
 */

#ifndef LDISCUCS_H_
#define LDISCUCS_H_

void lpage_send(void *handle, int codepage, char *buf, int len,
		int interactive);

void luni_send(void *handle, wchar_t * widebuf, int len, int interactive);

#endif /* LDISCUCS_H_ */
