/*
 * CommandReceiver.cpp
 *
 *  Created on: 2012-9-3
 *      Author: zhangbo
 */

#include "CommandReceiver.h"

#include <iostream>
#include <stdio.h>
#include "Util.h"
#include "md5.h"

CommandReceiver::CommandReceiver(RequestParameter& param) {
	this->listener = NULL;
	this->request.SetRequestParameter(param);
	this->request.SetListener(this);
	state = TOP_LEVEL;
}

CommandReceiver::~CommandReceiver() {
}

bool CommandReceiver::Do() {
	request.Get();
	return true;
}

bool CommandReceiver::OnReceive(int size, void* data) {
	char* dt = (char*) data;
	int i;
	for (i = 0; i < size; i++) {
		char nowchar = *(dt++);

		switch (state) {
		case TOP_LEVEL:
			if (nowchar == '<') {
				state = SEENLEFT;
				commandlen = 0;
			}
			break;
		case SEENLEFT:
			if (nowchar == '<') {
				commandlen = 0;
			} else if (nowchar == '>') {
				state = TOP_LEVEL;
				command[commandlen++] = 0;
				ReciveResolve(command);
			} else {
				if (commandlen < 1023) {
					command[commandlen++] = nowchar;
				} else {
					state = TOP_LEVEL;
					commandlen = 0;
				}
			}
			break;
		default:
			break;
		}
	}
	return true;
}

void CommandReceiver::ReciveResolve(string command) {
	unsigned int index = command.find_last_of(':');
	string cmd = command.substr(0, index);
	string md5 = command.substr(index + 1);
	unsigned char newmd5[16];
	Util::MD5((unsigned char *) cmd.c_str(), cmd.length(), newmd5);
	string temp = Util::Print(newmd5, 16);
	if (0 == temp.compare(md5)) {
		if (this->listener != NULL) {
			this->listener->Receive(cmd);
		}
	}
}

void CommandReceiver::SetListener(CommandListener* listener) {
	this->listener = listener;
}
