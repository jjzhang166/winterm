#include "Downloader.h"
#include <fstream>

Downloader::Downloader(void) {
	first = true;
}

Downloader::Downloader(const string& file) {
	this->first = true;
	this->file = file;
}

Downloader::~Downloader(void) {
}

bool Downloader::OnReceive(int size, void* data) {
	if (first && size > 0) {
		first = false;
		unlink(this->file.c_str());
	}
	ofstream of(file.c_str(), ios::binary | ios::app);
	of.write((const char*) data, size);
	of.close();
	return true;
}

void Downloader::SetFile(const string& file) {
	this->file = file;
}
