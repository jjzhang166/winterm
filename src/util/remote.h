/*
 * remote.h
 *
 *  Created on: 2013-1-25
 *      Author: zhangbo
 */

#ifndef REMOTE_H_
#define REMOTE_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned int remote_open(const char* ip, int port);

void remote_close(unsigned int handle);

unsigned int remote_recv(unsigned int handle, void* data, int maxlen);

unsigned int remote_send(unsigned int handle, const void* data, int len);

#ifdef __cplusplus
}
#endif

#endif /* REMOTE_H_ */
