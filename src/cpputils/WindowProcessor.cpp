/*
 * WindowProcessor.cpp
 *
 *  Created on: 2012-9-2
 *      Author: xia
 */

#include "WindowProcessor.h"

WindowProcessor::WindowProcessor() {
	// TODO Auto-generated constructor stub

}

WindowProcessor::~WindowProcessor() {
	// TODO Auto-generated destructor stub
}

void WindowProcessor::SetWindow(Window* win) {
	this->window = win;
}

void WindowProcessor::Run() {
	this->window->Process();
}
