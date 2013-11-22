#include "ThreadCreator.h"
#include <windows.h>

ThreadCreator::ThreadCreator(Runnable* runnable) {
	this->handle = NULL;
	this->runnable = runnable;
	this->autoDestroy = false;
}

ThreadCreator::ThreadCreator(Runnable* runnable, bool autoDestroy) {
	this->handle = NULL;
	this->runnable = runnable;
	this->autoDestroy = autoDestroy;
}

ThreadCreator::~ThreadCreator(void) {

}

DWORD WINAPI ThreadFunction(LPVOID pthread) {
	ThreadCreator* creator = (ThreadCreator*) pthread;
	creator->GetRunnable()->Run();
	if (creator->AutoDestroy()) {
		delete creator->GetRunnable();
		delete creator;
	}
	return 0;
}

Runnable* ThreadCreator::GetRunnable() {
	return this->runnable;
}

bool ThreadCreator::AutoDestroy() {
	return this->autoDestroy;
}

void* ThreadCreator::Start() {
	this->handle = ::CreateThread(NULL, 0, ThreadFunction, (LPVOID) this, 0, 0);
	return this->handle;
}

void* ThreadCreator::GetHandle() {
	return this->handle;
}

void* ThreadCreator::StartThread(Runnable* runnable) {
	return (new ThreadCreator(runnable, true))->Start();
}
