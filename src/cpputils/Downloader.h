#pragma once

#include "DataReceiver.h"
#include <string>

using namespace std;

class Downloader: public DataReceiver {
private:
	string file;
	bool first;

public:
	Downloader(void);
	Downloader(const string& file);
	virtual ~Downloader(void);

	void SetFile(const string& file);
	bool OnReceive(int size, void* data);
};

