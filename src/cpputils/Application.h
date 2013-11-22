/*
 * Application.h
 *
 *  Created on: 2012-8-29
 *      Author: zhangbo
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "Runnable.h"
#include <string>
#include <vector>
using namespace std;

class Application {
private:
	vector<string> arugments;
	string name;
	string path;
	bool single;

public:
	Application();
	Application(const string& name, const string& path);
	virtual ~Application();

	string GetName();
	void SetName(string name);

	string GetPath();
	void SetPath(string path);

	bool GetSingle();
	void SetSingle(bool single);

	void SetArguments(int argc, char* argv[]);
	void SetArguments(const vector<string>& args);
	vector<string>& GetArguments();

	bool Run();

	virtual bool Start() = 0;
};

#endif /* APPLICATION_H_ */
