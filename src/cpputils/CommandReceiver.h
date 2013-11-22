/*
 * CommandReceiver.h
 *
 *  Created on: 2012-9-3
 *      Author: zhangbo
 */

#ifndef COMMANDRECEIVER_H_
#define COMMANDRECEIVER_H_

#include "DataReceiver.h"
#include "LoopRunnable.h"
#include "HttpRequest.h"
#include "CommandListener.h"
#include <string>
using namespace std;

class CommandReceiver: DataReceiver, public LoopRunnable {
private:
	HttpRequest request;
	CommandListener* listener;

	enum {
		TOP_LEVEL, SEENLEFT
	} state;
	char command[1024];
	int commandlen;

	void ReciveResolve(string command);

	void RunCmd(string cmd);

public:
	CommandReceiver(RequestParameter& request);

	virtual ~CommandReceiver();

	bool Do();

	bool OnReceive(int size, void* data);

	void SetListener(CommandListener* listener);
};

#endif /* COMMANDRECEIVER_H_ */
