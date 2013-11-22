/*
 * LoopRunnable.h
 *
 *  Created on: 2012-6-28
 *      Author: zhangbo
 */

#ifndef LOOPRUNNABLE_H_
#define LOOPRUNNABLE_H_

#include "Runnable.h"

class LoopRunnable: public Runnable {
private:
	bool running;

public:
	LoopRunnable();
	virtual ~LoopRunnable();

	void Stop();

	void Run();

	virtual bool BeforeRun() {
		return true;
	}

	virtual bool Do() = 0;

	virtual bool AfterRun() {
		return true;
	}
};

#endif /* LOOPRUNNABLE_H_ */
