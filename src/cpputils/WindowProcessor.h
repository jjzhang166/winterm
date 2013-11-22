/*
 * WindowProcessor.h
 *
 *  Created on: 2012-9-2
 *      Author: xia
 */

#ifndef WINDOWPROCESSOR_H_
#define WINDOWPROCESSOR_H_

#include "Runnable.h"
#include "Window.h"

class WindowProcessor: public Runnable {
private:
	Window* window;
public:
	WindowProcessor();
	virtual ~WindowProcessor();

	void SetWindow(Window* win);

	void Run();
};

#endif /* WINDOWPROCESSOR_H_ */
