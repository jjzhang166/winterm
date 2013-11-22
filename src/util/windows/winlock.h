/*
 * winlock.h
 *
 *  Created on: 2013年11月11日
 *      Author: zhangbo
 */

#ifndef WINLOCK_H_
#define WINLOCK_H_

#ifdef __cplusplus
extern "C" {
#endif

void* create_lock();

void lock(void* lk);

void unlock(void* lk);

void release_lock(void* lk);

#ifdef __cplusplus
}
#endif

#endif /* WINLOCK_H_ */
