/*
 * Properties.cpp
 *
 *  Created on: 2012-7-7
 *      Author: zhangbo
 */

#include "Properties.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include "Util.h"
#include <stdio.h>
using namespace std;
#define MAX_LINE_LENGTH 4096

Properties::Properties() {
	// TODO Auto-generated constructor stub

}

Properties::~Properties() {
	// TODO Auto-generated destructor stub
}

string Properties::Get(const string& name, const string& def) {
	string ret;
	try {
		ret = this->properties.at(name);
	} catch (out_of_range& e) {
		ret = def;
	}
	return ret;
}

string Properties::GetString(const string& name, const string& def) {
	return this->Get(name, def);
}

void Properties::PutString(const string& name, const string& value) {
	this->Put(name, value);
}

unsigned long Properties::GetUnsignedLong(const string& name,
		unsigned long def) {
	string ret;
	try {
		ret = this->properties.at(name);
	} catch (out_of_range& e) {
		return def;
	}
	return strtoul(ret.c_str(), NULL, 10);
}

int Properties::GetInteger(const string& name, int def) {
	string ret;
	try {
		ret = this->properties.at(name);
	} catch (out_of_range& e) {
		return def;
	}
	return atoi(ret.c_str());
}

void Properties::PutInteger(const string& name, int value) {
	char str[1024];
	sprintf(str, "%d", value);
	this->properties[name] = str;
}

void Properties::Put(const string& name, const string& value) {
	this->properties[name] = value;
}

void Properties::Clear() {
	this->properties.clear();
}

map<string, string>::iterator Properties::Begin() {
	return this->properties.begin();
}

map<string, string>::iterator Properties::End() {
	return this->properties.end();
}

Properties Properties::Filter(const string& prefix) {
	Properties prop;
	map<string, string>::iterator it;
	for (it = properties.begin(); it != properties.end(); it++) {
		if (it->first.substr(0, prefix.length()) == prefix) {
			prop.Put(it->first.substr(prefix.length()), it->second);
		}
	}
	return prop;
}

bool Properties::SafeLoad(const string& file) {
	HANDLE hMutex = CreateMutex(NULL, FALSE, file.c_str());
	WaitForSingleObject(hMutex, INFINITE);

	//如果存在备份文件，则首先恢复
	if (Util::FileExists(file + "~")) {
		if (Util::FileExists(file)) {
			DeleteFile(file.c_str());
		}
		MoveFile((file + "~").c_str(), file.c_str());
	}

	//加载配置项
	this->Load(file);

	ReleaseMutex(hMutex);

	return true;
}

bool Properties::SafeSave(const string& file) {
	HANDLE hMutex = CreateMutex(NULL, FALSE, file.c_str());
	WaitForSingleObject(hMutex, INFINITE);

	//如果配置文件存在并且备份文件不存在，则首先备份
	if (Util::FileExists(file) && !Util::FileExists(file + "~")) {
		MoveFile(file.c_str(), (file + "~").c_str());
	}

	//保存配置文件
	this->Save(file);

	//删除备份文件
	DeleteFile((file + "~").c_str());

	ReleaseMutex(hMutex);

	return true;
}

bool Properties::Save(const string& file) {
	map<string, string>::iterator it;
	ofstream outf(file.c_str(), ios_base::out);
	for (it = properties.begin(); it != properties.end(); it++) {
		outf << it->first << "=" << it->second << endl;
		outf.flush();
	}
	outf.close();
	return true;
}

bool Properties::Load(const string& file) {
	ifstream inf(file.c_str()); //, ios_base::in);
	bool ret = this->Load(inf);
	inf.close();
	return ret;
}

bool Properties::Load(istream& inf) {
	this->Clear();
	char buf[MAX_LINE_LENGTH];
	inf.getline(buf, MAX_LINE_LENGTH);
	while (inf.good()) {
		string line = buf;
		if (line.length() > 0 && line.at(0) != '#') {
			unsigned int pos = line.find('=');
			if (pos != string::npos) {
				string name = Util::Trim(line.substr(0, pos));
				string value = Util::Trim(line.substr(pos + 1));
				properties[name] = value;
			}
		}
		inf.getline(buf, MAX_LINE_LENGTH);
	}
	return true;
}

bool Properties::Load(void* data, unsigned long size) {
	stringstream ss;
	ss.write((char*) data, size);
	return this->Load(ss);
}
