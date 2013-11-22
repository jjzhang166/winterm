/*
 * LoopRunnable.cpp
 *
 *  Created on: 2012-6-28
 *      Author: zhangbo
 */

#include "LoopRunnable.h"

LoopRunnable::LoopRunnable() {
	// TODO Auto-generated constructor stub

}

LoopRunnable::~LoopRunnable() {
	// TODO Auto-generated destructor stub
}

void LoopRunnable::Stop() {
	this->running = false;
}

void LoopRunnable::Run() {
	this->running = true;
	this->BeforeRun();
	while (this->running) {
		if (!this->Do()) {
			this->running = false;
			break;
		}
	}
	this->AfterRun();
}
