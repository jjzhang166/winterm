/*
 * MonitorCom.h
 *
 *  Created on: 2013-1-8
 *      Author: xia
 */

#ifndef MONITORCOM_H_
#define MONITORCOM_H_
#include "cpputils/Runnable.h"
class MonitorCom: public Runnable {
private:
	bool running;
public:
	MonitorCom();
	virtual ~MonitorCom();
	void Run();
	void Stop();
};

#endif /* MONITORCOM_H_ */
