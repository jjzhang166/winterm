#ifndef _PUB_H_
#define _PUB_H_

#include "types.h"
#include "stdio.h"

#ifdef __cplusplus
extern "C" {
#endif

void print_binary(char * tip, uint8 * buff, uint8 bits);

void print_hex(char * tip, uint8 * buff, uint8 bytes);

#ifdef __cplusplus
}
#endif

#endif//_PUB_H_
