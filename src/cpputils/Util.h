/*
 * Util.h
 *
 *  Created on: Jun 22, 2012
 *      Author: zhangbo
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <windows.h>
#include "HttpRequest.h"
#include <shlobj.h>
#include <shellapi.h>
#include <list>
#include <vector>
#include <string>
using namespace std;

class Util {
public:
	Util();
	virtual ~Util();

	static const char* CreateGUID();

	static vector<string> GetUUID(RequestParameter param, const string& path,
			const string& version);

	static unsigned int GetIniInt(string appName, string keyName, int def,
			string fileName);

	static string GetIniString(string appName, string keyName, string def,
			string fileName);

	static string ReadFile(string fileName, int length);

	static void WriteFile(string fileName, string content);

	static bool FileExists(string file);

	static vector<string> Split(string files, char chr);

	static string Join(vector<string> v, char chr);

	static void ShutDown();

	static void Reboot();

	static void ShellExecuteWait(const string& exe, const string& param,
			unsigned int age);

	static void ShellExecuteScreenSaverWait(string& exe, unsigned int age);

	static void ShellExecuteWait(const string& exe, const string& param);

	static void ExecuteCommand(const string& cmd, const string& dir,
			const string& file);

	static HANDLE ShellExecuteNoWait(const string& file);

	static int ProcessCount(string processName);

	static HANDLE ShellExecuteNoWait(const string& exe, const string& param,
			const string& dir = ".");

	static void KillProcess(string proName);

	static bool Download(string host, string url, int port, string file);

	static bool Download(RequestParameter param, string file);

	static bool CreateRootDir();

	static bool EnsureDir(string dir);

	static void Unzip(string zipFile, string path);

	static bool DeleteFile1(const char * lpszPath);

	static void CopyFile1(string sourceFile, string destinFile);

	static string ToLower(string strConvertToLower);

	static void MaxWindow(string title);

	static void MinWindow(string title);

	static void ExtWindow(string title);

	static void TopWindow(string title);

	static int DeleteFolder(const char * lpszPath);

	static void ImportRegisterFile(string file);

	static string FindFiles(const char *lpszPath, bool isTask);

	static string ToString(FILETIME time);

	static string GetModifiedTime(string file);

	static unsigned long GetSize(string file);

	static bool FilesRead(const char *lpszPath);

	static int MD5(const unsigned char *in, unsigned int len,
			unsigned char *out);

	static string Print(unsigned char* digest, unsigned int len);

	static bool MD5File(const char *filename, unsigned char* digest);

	static unsigned long GetPage(unsigned long total, unsigned long size);

	static bool ValidateFile(string file, unsigned long size, string md5);

	static string CurrentTime();

	static string CurrentDate();

	static unsigned long long ToUnsignedLongLong(unsigned char* dt);

	static HttpRequest& Request(const string& host, int port,
			const string& url);

	static string GetType(string surfix);

	static string ToString(int i);

	static string VersionUUID(const string& path);

	static string SystemUUID(const string& path);

	static string GetExeFile(const string& type);

	static string GetProcess(const string& type);

	static string GetSystemDir();

	static void PlayPhoto(string filePath, unsigned int age);

	static void SaveScreen(const string& filePath, void* hwnd = NULL);

	static string Trim(string str);

	static BOOL SelectFolder(HWND hWnd, LPSTR lpszFolder, LPCSTR lpszTitle,
			UINT ulFlag);

	static BOOL GetDesktopPath(char *pszDesktopPath);

	static int RSHash(string str);

	static string GenerateMach(char * localRegistCod, int type);

	static BOOL getMacAddress(char * macAddr);

	static BOOL ShellExecuteLogon(const char* user, const char* pass,
			const char* file, const char* path);

	static void GetCurrentPath(char *path);
};

#endif /* UTIL_H_ */
