/*
 * Logger.cpp
 *
 *  Created on: 2012-8-29
 *      Author: zhangbo
 */

#include "Logger.h"
#include "Util.h"
#include <fstream>
using namespace std;

Logger* Logger::logger = NULL;

Logger::Logger(const string& file) {
	this->file = file;
	this->level = LOG_INFO;
}

Logger::~Logger() {
	// TODO Auto-generated destructor stub
}

void Logger::SetFile(const string& file) {
	this->file = file;
}

void Logger::SetLevel(Logger::LogType type) {
	this->level = type;
}

void Logger::append(const string& file, const string& msg,
		Logger::LogType type) {
	ofstream outf(file.c_str(), ios_base::app);
	outf << type << " " << Util::CurrentTime() << " > " << msg << endl;
	outf.close();
}

void Logger::Log(const string& msg, Logger::LogType type) {
	if (type >= this->level) {
		Logger::append(this->file, msg, type);
	}
}

void Logger::Debug(const string& msg) {
	this->Log(msg, LOG_DEBUG);
}

void Logger::Info(const string& msg) {
	this->Log(msg, LOG_INFO);
}

void Logger::Warn(const string& msg) {
	this->Log(msg, LOG_WARN);
}

void Logger::Error(const string& msg) {
	this->Log(msg, LOG_ERROR);
}

void Logger::Fatal(const string& msg) {
	this->Log(msg, LOG_FATAL);
}

void Logger::SetDefault(const string& file, LogType level) {
	if (Logger::logger == NULL) {
		Logger::logger = new Logger(file);
	} else {
		Logger::logger->SetFile(file);
	}
	Logger::logger->SetLevel(level);
}

Logger* Logger::Default() {
	return Logger::logger;
}
