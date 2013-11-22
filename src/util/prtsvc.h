/*
 * prtsvc.h
 *
 *  Created on: 2013-1-22
 *      Author: zhangbo
 */

#ifndef PRTSVC_H_
#define PRTSVC_H_

#ifdef __cplusplus
extern "C" {
#endif

unsigned int prt_open(char* ip, int port);
unsigned long prt_write(unsigned int socket, const void* data, int len);
void prt_close(unsigned int socket);

#ifdef __cplusplus
}
#endif

#endif /* PRTSVC_H_ */
