/*
 * DataReceiver.h
 *
 *  Created on: 2012-8-29
 *      Author: zhangbo
 */

#ifndef DATARECEIVER_H_
#define DATARECEIVER_H_

class DataReceiver {
public:
	DataReceiver();
	virtual ~DataReceiver();

	virtual bool OnReceive(int size, void* data) = 0;
};

#endif /* DATARECEIVER_H_ */
