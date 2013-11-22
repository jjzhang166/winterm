/*
 * CommandListener.h
 *
 *  Created on: 2012-9-3
 *      Author: zhangbo
 */

#ifndef COMMANDLISTENER_H_
#define COMMANDLISTENER_H_

#include <string>
using namespace std;

class CommandListener {
public:
	CommandListener();
	virtual ~CommandListener();

	virtual void Receive(const string& command) = 0;
};

#endif /* COMMANDLISTENER_H_ */
