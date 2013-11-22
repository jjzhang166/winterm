/*
 * Application.cpp
 *
 *  Created on: 2012-8-29
 *      Author: zhangbo
 */

#include "Application.h"

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "Logger.h"
#include "Configure.h"
#include "Util.h"

Application::Application() {

}

Application::Application(const string& name, const string& path) {
	this->name = name;
	this->path = path;
	this->single = true;
}

Application::~Application() {
	// TODO Auto-generated destructor stub
}

string Application::GetName() {
	return this->name;
}

string Application::GetPath() {
	return this->path;
}

void Application::SetName(string name) {
	this->name = name;
}

void Application::SetPath(string path) {
	this->path = path;
}

bool Application::GetSingle() {
	return this->single;
}

void Application::SetSingle(bool single) {
	this->single = single;
}

void Application::SetArguments(const vector<string>& args) {
	this->arugments = args;
}

vector<string>& Application::GetArguments() {
	return this->arugments;
}

void Application::SetArguments(int argc, char* argv[]) {
	char temp[1024];
	for (int i = 0; i < argc; i++) {
		memset((void*) temp, 0x00, 1024);
		sprintf(temp, "%s", argv[i]);
		this->arugments.push_back(temp);
	}
}

bool Application::Run() {
	bool ret = false;
	HANDLE hMutex;
	if (this->single) {
		hMutex = CreateMutex(NULL, FALSE, this->GetName().c_str());
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			CloseHandle(hMutex);
			return false;
		}
	}

	string logfile = this->GetPath() + "/log.txt";
	Util::DeleteFile1(logfile.c_str());
	Logger::SetDefault(logfile, Logger::LOG_DEBUG);
	Configure::SetDefault(this->GetPath() + "/config.txt");
	ret = this->Start();

	if (this->single) {
		CloseHandle(hMutex);
	}

	return ret;
}
