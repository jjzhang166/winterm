/*
 * pairs.c
 *
 *  Created on: 2013-3-12
 *      Author: xia
 */

#include "pairs.h"
void init_pairs() {
	int i;
	for (i = 0; i < MAX_NUM; i++) {
		pairs[i].name = 0;
		memset(pairs[i].value, 0x00, KEY_LEN);

	}
}
BOOL put(int name, char *value) {
	int i;
	for (i = 0; i < MAX_NUM; i++) {
		if ((pairs[i].name == 0) || pairs[i].name == name) {
			pairs[i].name = name;
			memcpy(pairs[i].value, value, strlen(value));
			return TRUE;
		}
	}
	return FALSE;
}
char* get(int name) {
	int i = 0;
	for (i = 0; i < MAX_NUM; i++) {
		if (pairs[i].name == name) {
			return pairs[i].value;
		}
	}
	return 0;

}
