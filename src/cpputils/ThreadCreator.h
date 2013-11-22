#pragma once

#include "Runnable.h"

class ThreadCreator {
private:
	Runnable* runnable;
	void* handle;
	bool autoDestroy;

public:
	ThreadCreator(Runnable* runnable);
	ThreadCreator(Runnable* runnable, bool autoDestroy);
	~ThreadCreator(void);

	void* Start();

	void* GetHandle();

	Runnable* GetRunnable();
	bool AutoDestroy();

	static void* StartThread(Runnable* runnable);

};

