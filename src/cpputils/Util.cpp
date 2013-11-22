/*
 * Util.cpp
 *
 *  Created on: Jun 22, 2012
 *      Author: zhangbo
 */

#include "Util.h"
#include <windows.h>
#include "Downloader.h"
#include "HttpRequest.h"
#include "XUnzip.h"
#include <objbase.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <io.h>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <time.h>
#include <Iphlpapi.h>
#include "md5.h"
#include "Logger.h"
#include "../util/types.h"

using namespace std;

Util::Util() {

}

Util::~Util() {
}

const char* Util::CreateGUID() {
	static char buf[64] = { 0 };
	GUID guid;
	CoInitialize(NULL);
	if (S_OK == ::CoCreateGuid(&guid)) {
		_snprintf(buf, sizeof(buf),
				"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", guid.Data1,
				guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
				guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
				guid.Data4[6], guid.Data4[7]);
	}
	CoUninitialize();
	return (const char*) buf;
}

string Util::VersionUUID(const string& path) {
	string uuidFile = path + "version.txt";
	if (!Util::FileExists(uuidFile)) {
		string uuid = Util::CreateGUID();
		Util::WriteFile(uuidFile, uuid);
		return uuid;
	}
	return Util::ReadFile(uuidFile, 40);
}

string Util::SystemUUID(const string& path) {
	string uuidFile = path + "uuid.txt";
	if (!Util::FileExists(uuidFile)) {
		string uuid = Util::CreateGUID();
		Util::WriteFile(uuidFile, uuid);
		return uuid;
	}
	return Util::ReadFile(uuidFile, 40);
}

vector<string> Util::GetUUID(RequestParameter param, const string& path,
		const string& version) {
	string uuidFile = path + "uuid.txt";
	string uuid;
	srand((unsigned) time(NULL));
	param.Put("method", "boot");
	param.Put("version",
			version + "-" + Util::VersionUUID(path) + "-"
					+ Util::ReadFile(path + "reboot.txt", 36));

	while (true) {
		if (Util::Download(param, uuidFile)) {
			uuid = Util::ReadFile(uuidFile, 256);
			if (uuid.size() > 0) {
				break;
			}
		}
		Sleep(rand() % 10000);
	}
	vector<string> list = Util::Split(uuid, ':');
	Util::WriteFile(uuidFile, list[0]);
	return list;
}

unsigned int Util::GetIniInt(string appName, string keyName, INT def,
		string fileName) {
	return GetPrivateProfileInt(appName.c_str(), keyName.c_str(), def,
			fileName.c_str());
}

string Util::GetIniString(string appName, string keyName, string def,
		string fileName) {
	LPSTR lpReturned = new char[1024];
	GetPrivateProfileString(appName.c_str(), keyName.c_str(), def.c_str(),
			lpReturned, 1024, fileName.c_str());
	string ret = lpReturned;
	delete lpReturned;
	return ret;
}

string Util::ReadFile(string fileName, int length) {
	ifstream in(fileName.c_str());
	string ret;
	in >> ret;
	in.close();
	return ret;
}

void Util::WriteFile(string fileName, string content) {
	ofstream out(fileName.c_str());
	out << content;
	out.close();
}

bool Util::FileExists(string file) {
	int flag = access(file.c_str(), 0);
	if (flag == -1)
		return false;
	return true;
}

vector<string> Util::Split(string files, char chr) {
	vector<string> v;
	while (!files.empty()) {
		int pos = files.find_first_of(chr);
		if (pos == -1) {
			v.push_back(files);
			break;
		} else {
			v.push_back(files.substr(0, pos));
		}
		files = files.substr(pos + 1, files.length() - 1);
	}
	return v;
}

string Util::Join(vector<string> v, char chr) {
	string r;
	for (unsigned int i = 0; i < v.size(); i++) {
		r = r + v.at(i) + chr;
	}
	return r;
}

DWORD WINAPI GetVerInfo() {
	OSVERSIONINFO osver = { sizeof(OSVERSIONINFO) };
	GetVersionEx(&osver);
	return osver.dwMajorVersion;
}

void ShoutDown1(DWORD verInfo, UINT type) {
	try {
		if (verInfo >= 5) {
			HANDLE ToHandle;
			TOKEN_PRIVILEGES tkp;
			if (OpenProcessToken(GetCurrentProcess(),
					TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &ToHandle)) {
				LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
						&tkp.Privileges[0].Luid);
				tkp.PrivilegeCount = 1;
				tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				AdjustTokenPrivileges(ToHandle, FALSE, &tkp, 0,
						(PTOKEN_PRIVILEGES) NULL, 0);
				ExitWindowsEx(type | EWX_FORCE, 0);
			}
		} else {
			ExitWindowsEx(type | EWX_FORCE, 0);
		}
	} catch (...) {
		return;
	}
}

void Util::Reboot() {
	ShoutDown1(GetVerInfo(), EWX_REBOOT);
}

void Util::ShutDown() {
	ShoutDown1(GetVerInfo(), EWX_SHUTDOWN);
}

void Util::ShellExecuteWait(const string& exe, const string& param,
		unsigned int age) {
	HANDLE hProcess = ShellExecuteNoWait(exe, param);
	WaitForSingleObject(hProcess, age);
}

void Util::ShellExecuteScreenSaverWait(string& exe, unsigned int age) {
	string par = "";
	Util::ShellExecuteWait(exe, par, age);
}

void Util::ShellExecuteWait(const string& exe, const string& param) {
	HANDLE hProcess = Util::ShellExecuteNoWait(exe, param);
	WaitForSingleObject(hProcess, INFINITE);
}

void Util::ExecuteCommand(const string& cmd, const string& dir,
		const string& file) {
	string c = "/C " + cmd + " > \"" + file + "\"";
	HANDLE hProcess = Util::ShellExecuteNoWait("cmd", c.c_str(), dir.c_str());
	WaitForSingleObject(hProcess, INFINITE);
}

HANDLE Util::ShellExecuteNoWait(const string& exe, const string& param,
		const string& dir) {
	SHELLEXECUTEINFO si = { sizeof(SHELLEXECUTEINFO) };
	si.fMask = SEE_MASK_NOCLOSEPROCESS;
	si.lpVerb = "open";
	si.lpDirectory = dir.c_str();
	si.lpFile = exe.c_str();
	si.lpParameters = param.c_str();
	si.nShow = SW_SHOWNORMAL;
	::ShellExecuteEx(&si);
	return si.hProcess;
}

string Util::GetType(string surfix) {
	surfix = Util::ToLower(surfix);
	if (surfix == "doc" || surfix == "docx") {
		return "word";
	} else if (surfix == "xls" || surfix == "xlsx") {
		return "excel";
	} else if (surfix == "ppt" || surfix == "pptx") {
		return "ppt";
	} else if (surfix == "jpg" || surfix == "jpeg" || surfix == "bmp"
			|| surfix == "gif" || surfix == "png" || surfix == "dib") {
		return "picture";
	} else if (surfix == "rmvb" || surfix == "avi" || surfix == "mp4"
			|| surfix == "3gp" || surfix == "flv" || surfix == "mpg"
			|| surfix == "f4v" || surfix == "mpeg" || surfix == "vob"
			|| surfix == "mov" || surfix == "wmv") {
		return "video";
	} else if (surfix == "upd") {
		return "update";
	} else {
		return surfix;
	}
}

HANDLE Util::ShellExecuteNoWait(const string& file) {
	string param("");
	return ShellExecuteNoWait(file, param);
}

int Util::ProcessCount(string processName) {
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
			&hToken)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
			printf("Can't lookup privilege value.\n");
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
			printf("Can't adjust privilege value.\n");
		CloseHandle(hToken);
	}

	BOOL bResult;
	HANDLE hSnapshot;
	PROCESSENTRY32 ProcessEntry;
	vector<DWORD> vtPID;
	string strTemp;
	string strProcessName = Util::ToLower(processName);

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	bResult = Process32First(hSnapshot, &ProcessEntry);
	while (bResult) {
		strTemp = Util::ToLower(ProcessEntry.szExeFile);
		if (strProcessName.compare(strTemp) == 0) {
			vtPID.push_back(ProcessEntry.th32ProcessID);
		}
		bResult = Process32Next(hSnapshot, &ProcessEntry);
	}

	return vtPID.size();
}

void Util::KillProcess(string processName) {
	HANDLE hToken;
	if (Logger::Default()) {
		Logger::Default()->Debug("KIL:" + processName);
	}
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES,
			&hToken)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid))
			printf("Can't lookup privilege value.\n");
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL))
			printf("Can't adjust privilege value.\n");
		CloseHandle(hToken);
	}

	BOOL bResult;
	HANDLE hSnapshot;
	PROCESSENTRY32 ProcessEntry;
	vector<DWORD> vtPID;
	string strTemp;
	string strProcessName = Util::ToLower(processName);

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	ProcessEntry.dwSize = sizeof(PROCESSENTRY32);
	bResult = Process32First(hSnapshot, &ProcessEntry);
	while (bResult) {
		strTemp = Util::ToLower(ProcessEntry.szExeFile);
		if (strProcessName.compare(strTemp) == 0) {
			vtPID.push_back(ProcessEntry.th32ProcessID);
		}
		bResult = Process32Next(hSnapshot, &ProcessEntry);
	}
	bResult = FALSE;
	vector<DWORD>::iterator it = vtPID.begin();
	for (; it != vtPID.end(); ++it) {
		if (*it) {
			HANDLE hProcess;
			hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *it);
			if (hProcess)
				bResult = TerminateProcess(hProcess, 0);
			if (!bResult)
				break;
		}
	}
}

bool Util::Download(string host, string url, int port, string file) {
	HttpRequest request;

	Downloader downloader;
	downloader.SetFile(file);
	request.SetListener(&downloader);

	request.SetHost(host);
	request.SetPort(port);
	request.SetUrl(url);
	return request.Get(10);
}

bool Util::Download(RequestParameter param, string file) {
	HttpRequest request;
	Downloader downloader;
	downloader.SetFile(file);
	request.SetListener(&downloader);
	request.SetRequestParameter(param);
	return request.Get(10);
}

bool Util::EnsureDir(string dir) {
	int rv = access(dir.c_str(), 0);
	if (rv == -1) {
		mkdir(dir.c_str());
	}
	return true;
}

string Util::FindFiles(const char *lpszPath, bool isTask) {
	char szFind[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	strcpy(szFind, lpszPath);
	strcat(szFind, "\\*.*");
	HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return "error";
	string ids;
	do {
		if (strcmp(FindFileData.cFileName, ".") != 0
				&& strcmp(FindFileData.cFileName, "..") != 0) {
			char size[20];
			string filepath = lpszPath;
			filepath += "\\";
			filepath += FindFileData.cFileName;
//			if (strlen(FindFileData.cFileName) != 36) {
//
//				if (!isTask) {
//					Util::DeleteFile1(filepath.c_str());
//				} else {
//					Util::DeleteFolder(filepath.c_str());
//					RemoveDirectory(filepath.c_str());
//				}
//				continue;
//			}

			if (!isTask) {
				unsigned long x;
				x = Util::GetSize(filepath);
				sprintf(size, "%lu", x);
			}

			if (isTask) {
				ids += FindFileData.cFileName;
				ids += ";";
			} else {
				ids += FindFileData.cFileName;
				ids += "::";
				ids += size;
				ids += "::";
				ids += Util::GetModifiedTime(filepath);
				ids += ";";
			}
		}
	} while (::FindNextFile(hFind, &FindFileData));
	::FindClose(hFind);

	return ids;
}

bool Util::FilesRead(const char *lpszPath) {
	char szFind[MAX_PATH];
	string filename;
	WIN32_FIND_DATA FindFileData;
	strcpy(szFind, lpszPath);
	strcat(szFind, "\\*.*");
	HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return false;
	do {
		if (strcmp(FindFileData.cFileName, ".") != 0
				&& strcmp(FindFileData.cFileName, "..") != 0) {
			string filepath = lpszPath;
			filepath += "\\";
			filepath += FindFileData.cFileName;
			DWORD dwattr = GetFileAttributesA(filepath.c_str());
			SetFileAttributes(filepath.c_str(),
					dwattr & ~FILE_ATTRIBUTE_READONLY);
		}
	} while (::FindNextFile(hFind, &FindFileData));
	::FindClose(hFind);
	return true;
}

bool Util::DeleteFile1(const char * lpszPath) {
	DWORD dwattr = GetFileAttributesA(lpszPath);
	SetFileAttributes(lpszPath, dwattr & ~FILE_ATTRIBUTE_READONLY);
	return DeleteFile(lpszPath);
}

int Util::DeleteFolder(const char * lpPath) {
	char szFind[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	strcpy(szFind, lpPath);
	strcat(szFind, "\\*.*");
	HANDLE hFind = ::FindFirstFile(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return -1;
	do {
		if (strcmp(FindFileData.cFileName, ".") != 0
				&& strcmp(FindFileData.cFileName, "..") != 0) {
			string szfile = lpPath;
			szfile += "\\";
			szfile += FindFileData.cFileName;

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				DeleteFolder(szfile.c_str());
			}

			Util::DeleteFile1(szfile.c_str());
			RemoveDirectory(szfile.c_str());
		}
	} while (::FindNextFile(hFind, &FindFileData));

	::FindClose(hFind);

	return 0;
}
string Util::ToString(FILETIME time) {
	SYSTEMTIME sys;
	::FileTimeToSystemTime(&time, &sys);
	char str[20];
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", sys.wYear, sys.wMonth,
			sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);
	return str;
}

string Util::GetModifiedTime(string file) {
	FILETIME fileTime;
	HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	GetFileTime(hFile, NULL, NULL, &fileTime);
	FILETIME localTime;
	FileTimeToLocalFileTime(&fileTime, &localTime);
	CloseHandle(hFile);
	return Util::ToString(localTime);
}

unsigned long Util::GetSize(string file) {
	HANDLE hFile = CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned long h;
	unsigned long l = GetFileSize(hFile, &h);
	//unsigned long long size = (h << 32) + l;
	CloseHandle(hFile);
	return l;
}

void Util::Unzip(string zipFile, string path) {
	ZIPENTRY ze;
	memset(&ze, 0, sizeof(ze));

	HZIP hz = OpenZip((void*) zipFile.c_str(), 0, ZIP_FILENAME);
	ZRESULT zr = GetZipItem(hz, -1, &ze);

	if (zr == ZR_OK) {
		GetZipItem(hz, -1, &ze);
		int numitems = ze.index;

		char file[1024];
		memset(file, 0, 1024 * sizeof(char));
		for (int i = 0; i < numitems; i++) {
			GetZipItem(hz, i, &ze);
			sprintf(file, "%s%s", path.c_str(), ze.name);
			if (Util::FileExists(file)) {
				if (!Util::DeleteFile1(file)) {
					printf("cannot delete file: %s", file);
				}
			}
			if (!Util::FileExists(file)) {
				HANDLE hFile = ::CreateFile((const TCHAR*) file, GENERIC_WRITE,
						0, NULL, CREATE_ALWAYS, ze.attr, NULL);
				ZRESULT zr = UnzipItem(hz, i, hFile, 0, ZIP_HANDLE);
				if (zr != ZR_OK) {
					printf("error(%ld) unzip: %s", zr, ze.name);
				}
				CloseHandle(hFile);
			}
			DWORD dwattr = GetFileAttributesA(file);
			SetFileAttributes(file, dwattr & ~FILE_ATTRIBUTE_READONLY);
		}
	}

	CloseZip(hz);
}

void Util::CopyFile1(string srcFile, string desFile) {
	CopyFile(srcFile.c_str(), desFile.c_str(), false);
}

string Util::ToLower(string str) {
	transform(str.begin(), str.end(), str.begin(), (int (&)(int))tolower);return str;
}

BOOL CALLBACK EnumMax(HWND hWnd, LPARAM param) {
	char title[1024];
	GetWindowText(hWnd, title, 1024);
	string* content = (string*) param;
	if (strstr(title, content->c_str()) != NULL) {
		SendMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
	}
	return true;
}

BOOL CALLBACK EnumMin(HWND hWnd, LPARAM param) {
	char title[1024];
	GetWindowText(hWnd, title, 1024);
	string* content = (string*) param;
	if (strstr(title, content->c_str()) != NULL) {
		SendMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	}
	return true;
}

BOOL CALLBACK EnumExt(HWND hWnd, LPARAM param) {
	char title[1024];
	GetWindowText(hWnd, title, 1024);
	string* content = (string*) param;
	if (strstr(title, content->c_str()) != NULL) {
		SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
	}
	return true;
}

BOOL CALLBACK EnumTop(HWND hWnd, LPARAM param) {
	char title[1024];
	GetWindowText(hWnd, title, 1024);
	string* content = (string*) param;
	if (strstr(title, content->c_str()) != NULL) {
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	return true;
}

void Util::MaxWindow(string title) {
	EnumWindows(EnumMax, (LPARAM) &title);
}

void Util::MinWindow(string title) {
	EnumWindows(EnumMin, (LPARAM) &title);
}

void Util::ExtWindow(string title) {
	EnumWindows(EnumExt, (LPARAM) &title);
}
void Util::TopWindow(string title) {
	EnumWindows(EnumTop, (LPARAM) &title);
}

void Util::ImportRegisterFile(string file) {
	string exe = "regedit.exe";
	string par = "/s \"" + file + "\"";
	Util::ShellExecuteWait(exe, par, 1000);
}

int Util::MD5(const unsigned char *in, unsigned int len, unsigned char *out) {
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, in, len);
	MD5Final(out, &context);
	return 0;
}

string Util::Print(unsigned char* digest, unsigned int len) {
	char* tmp = new char[2 * len + 1];
	char* cur = tmp;
	for (unsigned int i = 0; i < len; i++) {
		sprintf(cur, "%02x", digest[i]);
		cur += 2;
	}
	tmp[2 * len] = 0;
	return tmp;
}

bool Util::MD5File(const char *filename, unsigned char* digest) {
	FILE *file;
	MD5_CTX context;
	int len;
	unsigned char buffer[1024];

	if ((file = fopen(filename, "rb")) != NULL) {
		MD5Init(&context);
		while ((len = fread(buffer, 1, 1024, file)) != 0) {
			MD5Update(&context, buffer, len);
		}
		MD5Final(digest, &context);
		string temp = Print(digest, 16);
		fclose(file);
		return true;
	}
	return false;
}

unsigned long Util::GetPage(unsigned long total, unsigned long size) {
	unsigned long temp = total / size;
	if ((total % size) != 0) {
		return temp + 1;
	}
	return temp;
}

string Util::CurrentTime() {
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	char str[20];
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", sys.wYear, sys.wMonth,
			sys.wDay, sys.wHour, sys.wMinute, sys.wSecond);
	return str;
}

string Util::CurrentDate() {
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	char str[20];
	sprintf(str, "%04d;%02d;%02d;%01d;", sys.wYear, sys.wMonth, sys.wDay,
			sys.wDayOfWeek);

	return str;
}

bool Util::ValidateFile(string file, unsigned long size, string md5) {
	unsigned char newmd5[16];
	MD5File(file.c_str(), newmd5);
	string temp = Print(newmd5, 16);
	if (0 == memcmp(md5.c_str(), temp.c_str(), 32)) {
		return true;
	}
	return false;
}

unsigned long long Util::ToUnsignedLongLong(unsigned char* dt) {
	unsigned long long result = 0, temp = 0;
	for (int i = 0; i < 8; i++) {
		temp = *(dt + i);
		temp <<= (7 - i) * 8;
		result += temp;
	}
	return result;
}

HttpRequest& Util::Request(const string& host, int port, const string& url) {
	HttpRequest* p = new HttpRequest();
	p->SetHost(host);
	p->SetPort(port);
	p->SetUrl(url);
	return *p;
}

string Util::ToString(int i) {
	char* chrs = new char[10];
	sprintf(chrs, "%03d", i);
	string ret = chrs;
	delete[] chrs;
	return ret;
}

string Util::GetExeFile(const string& type) {
	if (type == "chrome") {
		return "C:\\Documents and Settings\\Administrator\\Local Settings\\Application Data\\Google\\Chrome\\Application\\chrome.exe";
	} else if (type == "video") {
		return "C:\\Program Files\\The KMPlayer\\KMPlayer.exe";
	} else if (type == "gd") {
		return "C:\\Program Files\\Sursen\\SepReader\\Reader.exe";
	} else if (type == "word") {
		return "C:\\Program Files\\Microsoft Office\\OFFICE12\\WINWORD.EXE";
	} else if (type == "excel") {
		return "C:\\Program Files\\Microsoft Office\\OFFICE12\\EXCEL.EXE";
	} else if (type == "ppt") {
		return "C:\\Program Files\\Microsoft Office\\OFFICE12\\PPTVIEW.EXE";
	} else if (type == "pdf") {
		return "C:\\Program Files\\Adobe\\Reader 10.0\\Reader\\AcroRd32.exe";
	}
	return "";
}

string Util::GetProcess(const string& type) {
	if (type == "chrome") {
		return "chrome.exe";
	} else if (type == "picture") {
		return "rundll32.exe";
	} else if (type == "video") {
		return "kmplayer.exe";
	} else if (type == "gd") {
		return "reader.exe";
	} else if (type == "word") {
		return "winword.exe";
	} else if (type == "excel") {
		return "excel.exe";
	} else if (type == "ppt") {
		return "pptview.exe";
	} else if (type == "pdf") {
		return "acrord32.exe";
	}
	return "";
}

string Util::GetSystemDir() {
	return "C:\\WINDOWS\\system32";
}

void Util::PlayPhoto(string filePath, unsigned int age) {
	string systemDir = Util::GetSystemDir();
	string runDllDir = systemDir + "\\rundll32.exe";
	string param = systemDir + "\\shimgvw.dll,ImageView_Fullscreen " + filePath;

	HANDLE h = Util::ShellExecuteNoWait(runDllDir, param);
	Sleep(1000);
	keybd_event(VK_F11, 0, 0, 0);
	keybd_event(VK_F11, 0, KEYEVENTF_KEYUP, 0);
	WaitForSingleObject(h, age);
}

void Util::SaveScreen(const string& filePath, void* hwnd) {
	RECT rect;
	HWND wnd = hwnd == NULL ? GetDesktopWindow() : (HWND) hwnd;
	HDC srcDC = GetWindowDC(wnd);
	GetWindowRect(wnd, &rect);
	LONG nCxWnd = rect.right - rect.left;
	LONG nCyWnd = rect.bottom - rect.top;

	HDC hdcMem = CreateCompatibleDC(srcDC);
	HBITMAP hBitmap = CreateCompatibleBitmap(srcDC, nCxWnd, nCyWnd);
	SelectObject(hdcMem, hBitmap);
	BitBlt(hdcMem, 0, 0, nCxWnd, nCyWnd, srcDC, 0, 0, SRCCOPY);

	BITMAP bitmap;
	GetObject(hBitmap, sizeof(BITMAP), &bitmap);
	DWORD size = bitmap.bmWidthBytes * bitmap.bmHeight;

	HGLOBAL hMem = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, size);
	if (hMem == NULL) {
		return;
	}

	LPSTR lpData = (LPSTR) GlobalLock(hMem);

	BITMAPINFOHEADER bih;
	memset(&bih, 0, sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = bitmap.bmWidth;
	bih.biHeight = bitmap.bmHeight;
	bih.biPlanes = 1;
	bih.biBitCount = bitmap.bmBitsPixel;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = size;
	bih.biXPelsPerMeter = 0;
	bih.biYPelsPerMeter = 0;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	if (GetDIBits(hdcMem, hBitmap, 0, bih.biHeight, lpData, (BITMAPINFO*) &bih,
			DIB_RGB_COLORS) == 0) {
		GlobalFree(hMem);
		return;
	}

	BITMAPFILEHEADER bfh;
	memset(&bfh, 0, sizeof(BITMAPFILEHEADER));
	bfh.bfType = ((WORD) ('M' << 8) | 'B');
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;
	bfh.bfOffBits = sizeof(bih) + sizeof(bfh);
	bfh.bfSize = bfh.bfOffBits + size;

	FILE* pf = fopen(filePath.c_str(), "wb");
	fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, pf);
	fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, pf);
	fwrite(lpData, size, 1, pf);
	fflush(pf);
	fclose(pf);

	GlobalFree(hMem);
	DeleteDC(hdcMem);

}

string Util::Trim(string str) {
	char chr;
	unsigned int i, j;
	if (str.length() == 0) {
		return str;
	}
	for (i = 0; i < str.length(); i++) {
		chr = str.at(i);
		if (chr == '\t' || chr == ' ' || chr == '\r' || chr == '\n') {
			continue;
		}
		break;
	}
	for (j = str.length() - 1; j >= i; j--) {
		chr = str.at(j);
		if (chr == '\t' || chr == ' ' || chr == '\r' || chr == '\n') {
			continue;
		}
		break;
	}
	str = str.substr(i, j - i + 1);
	return str;
}

BOOL Util::SelectFolder(HWND hWnd, LPSTR lpszFolder, LPCSTR lpszTitle,
		UINT ulFlag) {
	BROWSEINFO bi;
	ITEMIDLIST *pidl;

	memset(&bi, 0, sizeof(bi));

	bi.hwndOwner = hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = lpszFolder;
	bi.lpszTitle = lpszTitle;
//	bi.ulFlags = BIF_RETURNONLYFSDIRS;BIF_NEWDIALOGSTYLE
	bi.ulFlags = ulFlag;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;

	pidl = SHBrowseForFolder(&bi);

	if (pidl) {
		SHGetPathFromIDList(pidl, lpszFolder);

		LPMALLOC lpMalloc;

		if (SUCCEEDED(SHGetMalloc(&lpMalloc))) {
			lpMalloc->Free(pidl);
			lpMalloc->Release();
		}

		return TRUE;
	}

	return FALSE;
}

BOOL Util::GetDesktopPath(char *pszDesktopPath) {
	LPITEMIDLIST ppidl = NULL;

	if (SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &ppidl) == S_OK) {
		BOOL flag = SHGetPathFromIDList(ppidl, pszDesktopPath);
		CoTaskMemFree(ppidl);
		return flag;
	}

	return FALSE;
}

int Util::RSHash(string str) {
	int b = 378551;
	int a = 63689;
	int hash = 0;

	for (unsigned int i = 0; i < str.length(); i++) {
		hash = hash * a + str[i];
		a = a * b;
	}
	return (hash & 0x7FFFFFFF);
}

BOOL Util::getMacAddress(char * macAddr) {
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;
	ULONG ulOutBufLen;
	pAdapterInfo = (PIP_ADAPTER_INFO) malloc(sizeof(IP_ADAPTER_INFO));
	ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc(ulOutBufLen);
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
		pAdapter = pAdapterInfo;
		while (pAdapter) {
			char addr[24];
			sprintf(addr, "%02x%02x%02x%02x%02x%02x", pAdapter->Address[0],
					pAdapter->Address[1], pAdapter->Address[2],
					pAdapter->Address[3], pAdapter->Address[4],
					pAdapter->Address[5]);
			pAdapter = pAdapter->Next;
			strcat(macAddr, addr);
		}
		return true;
	} else {
		printf("Call to GetAdaptersInfo failed.\n");
		return false;
	}
}

wchar_t* Ansi2WChar(LPCSTR pszSrc) {
	int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR) pszSrc, strlen(pszSrc),
			0, 0);
	if (nSize <= 0)
		return NULL;
	WCHAR *pwszDst = new WCHAR[nSize + 1];
	if (NULL == pwszDst)
		return NULL;
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR) pszSrc, strlen(pszSrc), pwszDst,
			nSize);
	pwszDst[nSize] = 0;
	if (pwszDst[0] == 0xFEFF)
		for (int i = 0; i < nSize; i++)
			pwszDst[i] = pwszDst[i + 1];
	return pwszDst;
}

BOOL Util::ShellExecuteLogon(const char* user, const char* pass,
		const char* file, const char* path) {
	PROCESS_INFORMATION pi;
	STARTUPINFOW si = { sizeof(STARTUPINFOW), NULL, L"", NULL, 0, 0, 0, 0, 0, 0,
			0, STARTF_USESHOWWINDOW, 0, 0, NULL, 0, 0, 0 };
	si.wShowWindow = SW_SHOW;
	si.lpDesktop = NULL;
	ZeroMemory( &pi, sizeof(pi) );
	wchar_t* wuser = Ansi2WChar(user);
	wchar_t* wpass = Ansi2WChar(pass);
	wchar_t* wfile = Ansi2WChar(file);
	wchar_t* wpath = Ansi2WChar(path);
	BOOL ret = CreateProcessWithLogonW(wuser, L".", wpass, 0, NULL, wfile, 0,
			NULL, wpath, &si, &pi);
	printf("file:%s %d", file, GetLastError());
	delete wuser;
	delete wpass;
	delete wfile;
	delete wpath;
	return ret;
}

void Util::GetCurrentPath(char *path) {
	GetModuleFileName(NULL, path, MAX_PATH);
	(strrchr(path, '\\'))[0] = 0;
}
