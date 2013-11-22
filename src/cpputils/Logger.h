/*
 * Logger.h
 *
 *  Created on: 2012-8-29
 *      Author: zhangbo
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
using namespace std;

class Logger {

public:
	Logger(const string& file);
	virtual ~Logger();

	enum LogType {
		LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
	};

	void SetFile(const string& file);

	void SetLevel(LogType type);

	static void append(const string& file, const string& msg, LogType type);

	void Log(const string& msg, LogType type);

	void Debug(const string& msg);

	void Info(const string& msg);

	void Warn(const string& msg);

	void Error(const string& msg);

	void Fatal(const string& msg);

	static void SetDefault(const string& file, LogType level);

	static Logger* Default();

private:
	string file;
	LogType level;

	static Logger* logger;
};

#endif /* LOGGER_H_ */
