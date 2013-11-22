/*
 * MonitorCom.cpp
 *
 *  Created on: 2013-1-8
 *      Author: xia
 */

#include "MonitorCom.h"
#include "putty.h"
#include "windows/message.h"
#include <string>
using namespace std;
static char titlename[100];
string temp;

MonitorCom::MonitorCom() {
	// TODO Auto-generated constructor stub
	this->running = false;
	GetWindowTextA(hwnd, titlename, 100);
}

MonitorCom::~MonitorCom() {
	// TODO Auto-generated destructor stub
}

void MonitorCom::Run() {
	running = true;
	comchannel = 0;
	GetWindowTextA(hwnd, titlename, 100);
	while (running) {
		if ((comchannel != 0) && comselflag == 0) {
			temp = titlename;
			temp += CURRENTCOM;
			temp += comchannel;
			comselflag = 1;
			set_title(NULL, (char *) temp.c_str());
		} else if ((comchannel == 0) && comselflag == 1) {
			comselflag = 0;
			set_title(NULL, titlename);
		}
	}
}

void MonitorCom::Stop() {
	running = false;
}
