/*
 * pairs.h
 *
 *  Created on: 2013-3-12
 *      Author: xia
 */
#ifdef __cplusplus
extern "C" {

#endif
#ifndef PAIRS_H_
#define PAIRS_H

#include <windows.h>
#define MAX_NUM 256
#define KEY_LEN 10
typedef struct {
	int name;
	char value[KEY_LEN];
} Pairs_tag;
Pairs_tag pairs[MAX_NUM];
//GLOBAL Pairs pairs;

BOOL put(int name, char *value);
char* get(int name);
void init_pairs();
#endif /* PAIRS_H_ */

#ifdef __cplusplus
}
#endif
