/*
 * log.h
 *
 *  Created on: 2013-4-12
 *      Author: zhangbo
 */

#ifndef LOG_H_
#define LOG_H_

#define SOCK_SEND "SCK_SEND"
#define SOCK_RECV "SCK_RECV"
#define COM_INFO  "COM_INFO"
#define COM_OPEN  "COM_OPEN"
#define COM_SEND  "COM_SEND"
#define COM_RECV  "COM_RECV"
#define COM_CLOS  "COM_CLOS"
#define PRT_DATA  "PRT_DATA"
#define PRT_ENDS  "PRT_ENDS"
#define SYS_INFO  "SYS_INFO"
#define MSG_INFO  "MSG_INFO"

#ifdef __cplusplus
extern "C"
{
#endif

void log_open();

void log_close();

void log_set_enable(int enable);

void log_set_encrypt(int enable);

void log_write(const char* type, const void* data, unsigned long len);

void log_write_err(const char* type, const char* data, unsigned long err);

void log_write_str(const char* type, const char* data);

#ifdef  __cplusplus
}
#endif

#endif /* LOG_H_ */
