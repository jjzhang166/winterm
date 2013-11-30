/*
 * ConfigDialog.cpp
 *
 *  Created on: 2012-9-25
 *      Author: xia
 */
#define   NO_WIN32_LEAN_AND_MEAN

#include "ConfigDialog.h"
#include <stdio.h>
#include <iostream>
#include "../storage.h"
#include "../Setting.h"
#include <cpputils/Util.h>
#include <util/windows/shortcut.h>

#include "util.h"

extern "C" {
extern char localRegistKey[1024];
extern int tranKeyCode(char *keyStr, unsigned char *keyCodeStr);
extern unsigned char KeyCodeFXX[12][8];
extern int KeyCodeLenFXX[12];
extern int comHandle[4];
extern char sectionName[256];
}
static int backR;
static int backG;
static int backB;
static int fontR;
static int fontG;
static int fontB;
static int boderR;
static int boderG;
static int boderB;
static int cursonR;
static int cursonG;
static int cursonB;

int iItemIndex; //下拉列表框记录当前索引
char port[8];
unsigned int iCircle;
#define TABSHEETMAX 4
static HWND g_hTabSheet[TABSHEETMAX];
namespace terminal {

static Config config;

ConfigDialog::ConfigDialog() {
}

ConfigDialog::~ConfigDialog() {
}

void colours_tocfg1(HWND hwnd) {
	config.colours[0][0] = GetDlgItemInt(hwnd, IDC_FONTR, NULL, FALSE);
	config.colours[0][1] = GetDlgItemInt(hwnd, IDC_FONTG, NULL, FALSE);
	config.colours[0][2] = GetDlgItemInt(hwnd, IDC_FONTB, NULL, FALSE);
	config.colours[1][0] = GetDlgItemInt(hwnd, IDC_EDGER, NULL, FALSE);
	config.colours[1][1] = GetDlgItemInt(hwnd, IDC_EDGEG, NULL, FALSE);
	config.colours[1][2] = GetDlgItemInt(hwnd, IDC_EDGEB, NULL, FALSE);
	config.colours[2][0] = GetDlgItemInt(hwnd, IDC_BACKR, NULL, FALSE);
	config.colours[2][1] = GetDlgItemInt(hwnd, IDC_BACKG, NULL, FALSE);
	config.colours[2][2] = GetDlgItemInt(hwnd, IDC_BACKB, NULL, FALSE);
	config.colours[5][0] = GetDlgItemInt(hwnd, IDC_CURSONR, NULL, FALSE);
	config.colours[5][1] = GetDlgItemInt(hwnd, IDC_CURSONG, NULL, FALSE);
	config.colours[5][2] = GetDlgItemInt(hwnd, IDC_CURSONB, NULL, FALSE);
}

void cfg_tocolours() {
	cursonR = config.colours[5][0];
	cursonG = config.colours[5][1];
	cursonB = config.colours[5][2];

	fontR = config.colours[0][0];
	fontG = config.colours[0][1];
	fontB = config.colours[0][2];

	backR = config.colours[2][0];
	backG = config.colours[2][1];
	backB = config.colours[2][2];

	boderR = config.colours[1][0];
	boderG = config.colours[1][1];
	boderB = config.colours[1][2];
}

void colours_resumvalue() {
	backR = 0;
	backG = 128;
	backB = 64;
	fontR = 255;
	fontG = 255;
	fontB = 0;
	boderR = 255;
	boderG = 255;
	boderB = 255;
	cursonR = 255;
	cursonG = 0;
	cursonB = 0;
}

static void Save() {
	char filepath[1024];
	sprintf(filepath, "%s\\%s.txt", currentpath, sectionName);
	Setting::SavePropertiesFile(filepath, &config);
	Setting::SaveConfigsFile(configpath, &config);
}

void LoadConfigAndProperties() {
	char filepath[1024];
	sprintf(filepath, "%s\\%s.txt", currentpath, sectionName);
	Setting::LoadPropertiesFile(filepath, &config);
	Setting::LoadConfigsFile(configpath, &config);
}

static int CALLBACK RegProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	char InputKey[128];

	switch (msg) {
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		CloseHandle(hMutex);
		exit(1);
	case WM_INITDIALOG:
		SetDlgItemText(hwnd, IDC_STATICMACHCODE, config.registcod);
		SetWindowTextA(hwnd, APPLICATION_NAME);
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MACHCODE:
			if (OpenClipboard(hwnd)) {
				HGLOBAL hmem = GlobalAlloc(GMEM_DDESHARE, 256);
				LPVOID pmem = GlobalLock(hmem);
				EmptyClipboard();
				memcpy(pmem, config.registcod, strlen(config.registcod));
				SetClipboardData(CF_TEXT, hmem);
				CloseClipboard();
				GlobalUnlock(hmem);
				MessageBox(NULL, MSG_COPY_SUCCESS, APPLICATION_NAME,
				MB_OK | MB_ICONINFORMATION);
			}
			break;
		case IDC_REG:
			memset(InputKey, 0x00, 128);
			GetDlgItemText(hwnd, IDC_REGCODE, InputKey, 128);
			if (memcmp(localRegistKey, InputKey,
					strlen(localRegistKey) > strlen(InputKey) ?
							strlen(localRegistKey) : strlen(InputKey)) != 0) {
				MessageBox(NULL, MSG_ERROR_REGKEY, APPLICATION_NAME,
				MB_OK | MB_ICONWARNING);
				memset(InputKey, 0x00, 128);
				SetDlgItemText(hwnd, IDC_REGCODE, InputKey);
				return 0;
			}
			strcpy(config.registkey, localRegistKey);
			Save();
			EndDialog(hwnd, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			CloseHandle(hMutex);
			exit(1);
			break;
		}
		return 0;
	}
	return 0;
}

void ConfigDialog::ShowReg(HWND hwnd) {
	DialogBox(hinst, MAKEINTRESOURCE(IDD_REG), hwnd, RegProc);
}

int InputIPSuccess(HWND hwnd, INT IPID, INT PORTID, Config * cfg) {
	char textIPValue[32]; //存放窗体中IP值
	unsigned int textPortValue;
	GetDlgItemText(hwnd, IPID, textIPValue, 32);
	textPortValue = GetDlgItemInt(hwnd, PORTID, NULL, false);
	if (strcmp("0.0.0.0", textIPValue) == 0) {
		MessageBox(NULL, WARNING_HOST_EMPTY, APPLICATION_NAME,
		MB_OK | MB_ICONWARNING | MB_DEFBUTTON1);
		return 0;
	}
	if (textPortValue > 65535) {
		textPortValue = 65535;
	}
	cfg->port = textPortValue;
	memcpy(cfg->host, textIPValue, 32);
	return 1;
}

void keypro_resumvalue() {
	strcpy(config.keycodef[1], "^[OP");
	strcpy(config.keycodef[2], "^[OQ");
	strcpy(config.keycodef[3], "^[OR");
	strcpy(config.keycodef[4], "^[OS");
	strcpy(config.keycodef[5], "^[Ot");
	strcpy(config.keycodef[6], "^[Ou");
	strcpy(config.keycodef[7], "^[Ov");
	strcpy(config.keycodef[8], "^[Ol");
	strcpy(config.keycodef[9], "^[Ow");
	strcpy(config.keycodef[10], "^[Ox");
	strcpy(config.keycodef[11], "^[Oy");
	strcpy(config.keycodef[12], "");
}

void keypro_init(HWND hwnd) {
	SetDlgItemText(hwnd, IDC_EDITF1, config.keycodef[1]);
	SetDlgItemText(hwnd, IDC_EDITF2, config.keycodef[2]);
	SetDlgItemText(hwnd, IDC_EDITF3, config.keycodef[3]);
	SetDlgItemText(hwnd, IDC_EDITF4, config.keycodef[4]);
	SetDlgItemText(hwnd, IDC_EDITF5, config.keycodef[5]);
	SetDlgItemText(hwnd, IDC_EDITF6, config.keycodef[6]);
	SetDlgItemText(hwnd, IDC_EDITF7, config.keycodef[7]);
	SetDlgItemText(hwnd, IDC_EDITF8, config.keycodef[8]);
	SetDlgItemText(hwnd, IDC_EDITF9, config.keycodef[9]);
	SetDlgItemText(hwnd, IDC_EDITF10, config.keycodef[10]);
	SetDlgItemText(hwnd, IDC_EDITF11, config.keycodef[11]);
	SetDlgItemText(hwnd, IDC_EDITF12, config.keycodef[12]);
}

void dlg_to_cfg(HWND hwnd) {
	GetDlgItemText(hwnd, IDC_EDITF1, config.keycodef[1], 1024);
	GetDlgItemText(hwnd, IDC_EDITF2, config.keycodef[2], 1024);
	GetDlgItemText(hwnd, IDC_EDITF3, config.keycodef[3], 1024);
	GetDlgItemText(hwnd, IDC_EDITF4, config.keycodef[4], 1024);
	GetDlgItemText(hwnd, IDC_EDITF5, config.keycodef[5], 1024);
	GetDlgItemText(hwnd, IDC_EDITF6, config.keycodef[6], 1024);
	GetDlgItemText(hwnd, IDC_EDITF7, config.keycodef[7], 1024);
	GetDlgItemText(hwnd, IDC_EDITF8, config.keycodef[8], 1024);
	GetDlgItemText(hwnd, IDC_EDITF9, config.keycodef[9], 1024);
	GetDlgItemText(hwnd, IDC_EDITF10, config.keycodef[10], 1024);
	GetDlgItemText(hwnd, IDC_EDITF11, config.keycodef[11], 1024);
	GetDlgItemText(hwnd, IDC_EDITF12, config.keycodef[12], 1024);
}

void ConfigDialog::set_cfgto_uchar() {
	for (int i = 1; i < KEYCODEFXX_MAX_NUM; i++) {
		KeyCodeLenFXX[i] = 0;
		for (size_t j = 0; j < strlen(config.keycodef[i]); j++) {
			if ((config.keycodef[i][j] == 0x5E)
					&& (config.keycodef[i][j + 1]) == 0x5B) {
				j++;
				KeyCodeFXX[i - 1][KeyCodeLenFXX[i - 1]++] = 0X1B;
			} else
				KeyCodeFXX[i - 1][KeyCodeLenFXX[i - 1]++] =
						config.keycodef[i][j];
		}
	}
}

void ComAddText(HWND hwnd, int ID, LPARAM pCOMNumber) {
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, pCOMNumber);
}
void ConfigDialog::AddComCB(HWND hwnd, char*pCOMNumber) {
	ComAddText(hwnd, IDC_COM1Y, (LPARAM) pCOMNumber);
	ComAddText(hwnd, IDC_COM2Z, (LPARAM) pCOMNumber);
	ComAddText(hwnd, IDC_COM3X, (LPARAM) pCOMNumber);
	ComAddText(hwnd, IDC_COM4U, (LPARAM) pCOMNumber);
	ComAddText(hwnd, IDC_COM5W, (LPARAM) pCOMNumber);
	ComAddText(hwnd, IDC_COM6V, (LPARAM) pCOMNumber);
	ComAddText(hwnd, IDC_PRINTCOM, (LPARAM) pCOMNumber);
}

void ComSetText(HWND hwnd, int cfg, int ID) {
	char ComValue[8];
	sprintf(ComValue, "COM%d", cfg);
	SendMessage(GetDlgItem(hwnd, ID), WM_SETTEXT, 0, (LPARAM) TEXT(ComValue));
}

void ConfigDialog::InitCom(HWND hwnd) {
	ComSetText(hwnd, config.com01y, IDC_COM1Y);
	ComSetText(hwnd, config.com02z, IDC_COM2Z);
	ComSetText(hwnd, config.com03x, IDC_COM3X);
	ComSetText(hwnd, config.com04u, IDC_COM4U);
	ComSetText(hwnd, config.com05w, IDC_COM5W);
	ComSetText(hwnd, config.com06v, IDC_COM6V);
	ComSetText(hwnd, config.printcom, IDC_PRINTCOM);

}
void EnableDlgPrinter(HWND hwnd, bool enable) {
	ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), enable);
	ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), enable);
	ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), enable);
	ShowWindow(GetDlgItem(hwnd, IDC_SSHARE), enable);
	ShowWindow(GetDlgItem(hwnd, IDC_EPSHARE), enable);
}
void InitPrintCombox(HWND hwnd) {
	EnableDlgPrinter(hwnd, SW_HIDE);
	SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
			(LPARAM) DRIVEPRINT);

	SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
			(LPARAM) SERIALPRINT);
	if (config.allowrepeat == 1) {
		SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
				(LPARAM) SERVICEPRINT);

	} else if (config.allowrepeat == 0) {
		SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
				(LPARAM) DIRECTPRINT);

	}
	SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
			(LPARAM) FILEPRINT);
	if (config.printmode == 3) {
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(FILEPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, FILEPDIR);
		SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, FILENAME);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_SSHARE), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_EPSHARE), SW_SHOW);
		SetDlgItemTextA(hwnd, IDC_PRINTERNAME, config.fprintname);
		SetDlgItemTextA(hwnd, IDC_EPSHARE, config.fpshare);
	} else if ((config.printmode == 2) && (config.allowrepeat == 1)) {
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(SERVICEPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, SERVICEPDIR);
	} else if ((config.printmode == 2) && (config.allowrepeat == 0)) {
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(DIRECTPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, DIRECTPDIR);
//		ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_HIDE);
	} else if (config.printmode == 1) {
		SetDlgItemTextA(hwnd, IDC_SPDIR, SERIALPDIR);
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(SERIALPRINT));
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_SHOW);
//		ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_HIDE);
		SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTCOM);
	} else if (config.printmode == 0) {
		SendMessageA(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(DRIVEPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, DRIVEPRINT);
		SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTERNAME);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
//		ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_HIDE);
		SetDlgItemTextA(hwnd, IDC_PRINTERNAME, config.printer);
	}

}

int InitComCombox(HWND hwnd) {
	HKEY hKey;
	DWORD MaxValueLength;
	DWORD dwValueNumber;
	DWORD cchValueName, dwValueSize = 6;
	char pValueName[32], pCOMNumber[32];
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0,
	KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
		return 0;
	}
	if (RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
			&dwValueNumber, &MaxValueLength, NULL, NULL, NULL) != ERROR_SUCCESS) {
		return 0;
	}
	for (iCircle = 0; iCircle < dwValueNumber; iCircle++) {
		cchValueName = MaxValueLength + 1;
		dwValueSize = 6;
		long lReg = RegEnumValueA(hKey, iCircle, pValueName, &cchValueName,
		NULL, NULL, NULL, NULL);
		if ((lReg != ERROR_SUCCESS) && (lReg != ERROR_NO_MORE_ITEMS)) {
			continue;
		}
		RegQueryValueExA(hKey, pValueName, NULL, NULL, (LPBYTE) pCOMNumber,
				&dwValueSize);
		ConfigDialog::AddComCB(hwnd, pCOMNumber);
	}
	return 1;
}

void EnableDlgAutoLogin(HWND hwnd, BOOL enable) {
	EnableWindow(GetDlgItem(hwnd, IDC_LOGINPROMPT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_PASSWORDPROMPT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_USERNAME), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), enable);
}

static void ShowComName(HWND hwnd) {
	SetDlgItemTextA(hwnd, IDC_SCOM1Y, config.comname01y);
	SetDlgItemTextA(hwnd, IDC_SCOM2Z, config.comname02z);
	SetDlgItemTextA(hwnd, IDC_SCOM3X, config.comname03x);
	SetDlgItemTextA(hwnd, IDC_SCOM4U, config.comname04u);
	SetDlgItemTextA(hwnd, IDC_SCOM5W, config.comname05w);
	SetDlgItemTextA(hwnd, IDC_SCOM6V, config.comname06v);
}

void ConfigDialog::InitTabSheet0(HWND hwnd) {

	SetDlgItemText(hwnd, IDC_IPADDRESSVALUE, config.host);
	SetDlgItemInt(hwnd, IDC_EDITPORT, config.port, false);
	SetDlgItemText(hwnd, IDC_SCREENNUM, config.screennum);
	CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2,
			config.height == 24 ? IDC_RADIO1 : IDC_RADIO2);
	CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4,
			config.width == 80 ? IDC_RADIO3 : IDC_RADIO4);
	if (config.autologin == 1) {
		CheckDlgButton(hwnd, IDC_AUTOLOGIN, BST_CHECKED);
		SetDlgItemText(hwnd, IDC_LOGINPROMPT, config.loginprompt);
		SetDlgItemText(hwnd, IDC_PASSWORDPROMPT, config.passwordprompt);
		SetDlgItemText(hwnd, IDC_USERNAME, config.autousername);
		SetDlgItemText(hwnd, IDC_PASSWORD, config.autopassword);

	} else {
		CheckDlgButton(hwnd, IDC_AUTOLOGIN, BST_UNCHECKED);
		EnableDlgAutoLogin(hwnd, FALSE);
	}
	InitComCombox(hwnd);
	ShowComName(hwnd);
	ConfigDialog::InitCom(hwnd);
	InitPrintCombox(hwnd);
	CheckDlgButton(hwnd, IDC_TCPKEEPALIVES, config.tcp_keepalives);
	SetDlgItemInt(hwnd, IDC_PINGINTERVAL, config.ping_interval, false);
}

static void InitComName(HWND hwnd) {
	SetDlgItemTextA(hwnd, IDC_ECOM1, config.comname01y);
	SetDlgItemTextA(hwnd, IDC_ECOM2, config.comname02z);
	SetDlgItemTextA(hwnd, IDC_ECOM3, config.comname03x);
	SetDlgItemTextA(hwnd, IDC_ECOM4, config.comname04u);
	SetDlgItemTextA(hwnd, IDC_ECOM5, config.comname05w);
	SetDlgItemTextA(hwnd, IDC_ECOM6, config.comname06v);
}
static void InitBandRateCom(HWND hwnd, int ID) {
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "600");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "1200");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "2400");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "4800");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "9600");
}

static void InitByteCom(HWND hwnd, int ID) {
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "8");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "7");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "6");
	SetDlgItemInt(hwnd, ID, 8, false);
}
static void InitStopCom(HWND hwnd, int ID) {
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "1");
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) "2");
	SetDlgItemInt(hwnd, ID, 1, false);
}
static void InitPariyCom(HWND hwnd, int ID) {
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) NONE);
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) ODD);
	SendDlgItemMessage(hwnd, ID, CB_ADDSTRING, 0, (LPARAM) EVEN);
	SetDlgItemTextA(hwnd, ID, NONE);
}

static void InitUseSet(HWND hwnd, int ID, int state) {
	if (state) {
		CheckDlgButton(hwnd, ID, BST_CHECKED);
	} else {
		CheckDlgButton(hwnd, ID, BST_UNCHECKED);
	}
}

static void InitComSetY(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDY, config.bandy, false);
	InitBandRateCom(hwnd, IDC_BANDY);
	InitByteCom(hwnd, IDC_BYTEY);
	InitStopCom(hwnd, IDC_STOPY);
	InitPariyCom(hwnd, IDC_PARIYY);
	InitUseSet(hwnd, IDC_USESETY, config.useselfsety);
}

static void InitComSetZ(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDZ, config.bandz, false);
	InitBandRateCom(hwnd, IDC_BANDZ);
	InitByteCom(hwnd, IDC_BYTEZ);
	InitStopCom(hwnd, IDC_STOPZ);
	InitPariyCom(hwnd, IDC_PARIYZ);
	InitUseSet(hwnd, IDC_USESETZ, config.useselfsetz);
}
static void InitComSetX(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDX, config.bandx, false);
	InitBandRateCom(hwnd, IDC_BANDX);
	InitByteCom(hwnd, IDC_BYTEX);
	InitStopCom(hwnd, IDC_STOPX);
	InitPariyCom(hwnd, IDC_PARIYX);
	InitUseSet(hwnd, IDC_USESETX, config.useselfsetx);
}
static void InitComSetU(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDU, config.bandu, false);
	InitBandRateCom(hwnd, IDC_BANDU);
	InitByteCom(hwnd, IDC_BYTEU);
	InitStopCom(hwnd, IDC_STOPU);
	InitPariyCom(hwnd, IDC_PARIYU);
	InitUseSet(hwnd, IDC_USESETU, config.useselfsetu);
}
static void InitComSetW(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDW, config.bandw, false);
	InitBandRateCom(hwnd, IDC_BANDW);
	InitByteCom(hwnd, IDC_BYTEW);
	InitStopCom(hwnd, IDC_STOPW);
	InitPariyCom(hwnd, IDC_PARIYW);
	InitUseSet(hwnd, IDC_USESETW, config.useselfsetw);
}
static void InitComSetV(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDV, config.bandv, false);
	InitBandRateCom(hwnd, IDC_BANDV);
	InitByteCom(hwnd, IDC_BYTEV);
	InitStopCom(hwnd, IDC_STOPV);
	InitPariyCom(hwnd, IDC_PARIYV);
	InitUseSet(hwnd, IDC_USESETV, config.useselfsetv);
}
static void InitComSet(HWND hwnd) {
	InitComSetY(hwnd);
	InitComSetZ(hwnd);
	InitComSetX(hwnd);
	InitComSetU(hwnd);
	InitComSetW(hwnd);
	InitComSetV(hwnd);
}

static void ResetComName(HWND hwnd) {
	SetDlgItemTextA(hwnd, IDC_ECOM1, COMNAME1);
	SetDlgItemTextA(hwnd, IDC_ECOM2, COMNAME2);
	SetDlgItemTextA(hwnd, IDC_ECOM3, COMNAME3);
	SetDlgItemTextA(hwnd, IDC_ECOM4, COMNAME4);
	SetDlgItemTextA(hwnd, IDC_ECOM5, COMNAME5);
	SetDlgItemTextA(hwnd, IDC_ECOM6, COMNAME6);
}

static void SaveComName(HWND hwnd) {
	GetDlgItemTextA(hwnd, IDC_ECOM1, config.comname01y, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM2, config.comname02z, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM3, config.comname03x, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM4, config.comname04u, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM5, config.comname05w, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM6, config.comname06v, NAME_LEN);
}

static void SaveBandRate(HWND hwnd) {
	config.bandy = GetDlgItemInt(hwnd, IDC_BANDY, NULL, false);
	config.bandz = GetDlgItemInt(hwnd, IDC_BANDZ, NULL, false);
	config.bandx = GetDlgItemInt(hwnd, IDC_BANDX, NULL, false);
	config.bandu = GetDlgItemInt(hwnd, IDC_BANDU, NULL, false);
	config.bandw = GetDlgItemInt(hwnd, IDC_BANDW, NULL, false);
	config.bandv = GetDlgItemInt(hwnd, IDC_BANDV, NULL, false);
}

static int CALLBACK SetComNameProc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		InitComName(hwnd);
		InitComSet(hwnd);
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RESETNAME:
			ResetComName(hwnd);
			break;
		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			break;
		case IDOK:
			SaveComName(hwnd);
			SaveBandRate(hwnd);
			EndDialog(hwnd, TRUE);
			break;
		case IDC_USESETY:
			if (IsDlgButtonChecked(hwnd, IDC_USESETY) == BST_CHECKED) {
				config.useselfsety = 1;

			} else {
				config.useselfsety = 0;
			}
			break;
		case IDC_USESETZ:
			if (IsDlgButtonChecked(hwnd, IDC_USESETZ) == BST_CHECKED) {
				config.useselfsetz = 1;

			} else {
				config.useselfsetz = 0;
			}
			break;
		case IDC_USESETX:
			if (IsDlgButtonChecked(hwnd, IDC_USESETX) == BST_CHECKED) {
				config.useselfsetx = 1;

			} else {
				config.useselfsetx = 0;
			}
			break;
		case IDC_USESETU:
			if (IsDlgButtonChecked(hwnd, IDC_USESETU) == BST_CHECKED) {
				config.useselfsetu = 1;

			} else {
				config.useselfsetu = 0;
			}
			break;
		case IDC_USESETW:
			if (IsDlgButtonChecked(hwnd, IDC_USESETW) == BST_CHECKED) {
				config.useselfsetw = 1;

			} else {
				config.useselfsetw = 0;
			}
			break;
		case IDC_USESETV:
			if (IsDlgButtonChecked(hwnd, IDC_USESETV) == BST_CHECKED) {
				config.useselfsetv = 1;

			} else {
				config.useselfsetv = 0;
			}
			break;
		}
		break;
	}
	Save();
	return 0;
}

void ConfigDialog::ShowSetComName(HWND hwnd) {
	DialogBox(hinst, MAKEINTRESOURCE(IDD_SETCOMNAME), hwnd, SetComNameProc);
}

static int CALLBACK TabSheet0Proc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		ConfigDialog::InitTabSheet0(hwnd);
		return true;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SETCOMNAME:
			ConfigDialog::ShowSetComName(hwnd);
			break;
		case IDC_PRINTMODE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				config.printmode = SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE),
				CB_GETCURSEL, 0, 0);
				EnableDlgPrinter(hwnd, SW_HIDE);
				if (config.printmode == 0) { //选择驱动打印
					SetDlgItemTextA(hwnd, IDC_SPDIR, DRIVEPDIR);
					SetDlgItemTextA(hwnd, IDC_PRINTERNAME, config.printer);
					SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTERNAME);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
				} else if (config.printmode == 1) {
					SetDlgItemTextA(hwnd, IDC_SPDIR, SERIALPDIR);
					SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTCOM);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_SHOW);
				} else if (config.printmode == 2) {
					if (config.allowrepeat == 1) {
						SetDlgItemTextA(hwnd, IDC_SPDIR, SERVICEPDIR);
					} else if (config.allowrepeat == 0) {
						SetDlgItemTextA(hwnd, IDC_SPDIR, DIRECTPDIR);
					}
				} else if (config.printmode == 3) {
					SetDlgItemTextA(hwnd, IDC_SPDIR, FILEPDIR);
					SetDlgItemTextA(hwnd, IDC_PRINTERNAME, config.fprintname);
					SetDlgItemTextA(hwnd, IDC_EPSHARE, config.fpshare);
					SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, FILENAME);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_SSHARE), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_EPSHARE), SW_SHOW);

				}
			}
			break;

		case IDC_RADIO1:
			config.height = 24;
			break;
		case IDC_RADIO2:
			config.height = 25;
			break;
		case IDC_RADIO3:
			config.width = 80;
			break;
		case IDC_RADIO4:
			config.width = 132;
			break;
		case IDC_TCPKEEPALIVES:
			if (IsDlgButtonChecked(hwnd, IDC_TCPKEEPALIVES) == BST_CHECKED) {
				config.tcp_keepalives = 1;
			} else {
				config.tcp_keepalives = 0;
			}
			break;
		case IDC_AUTOLOGIN:
			if (IsDlgButtonChecked(hwnd, IDC_AUTOLOGIN) == BST_CHECKED) {
				config.autologin = 1;
				EnableDlgAutoLogin(hwnd, TRUE);
			} else {
				config.autologin = 0;
				EnableDlgAutoLogin(hwnd, FALSE);
			}
			break;
		}
		break;
	}
	return false;
}

void SetupScrollbars() {
	RECT tempRect;
	BOOL bMaximized;

	//Max Vertical Scrolling is the difference between size
	//of the Whole Property Page with Controls and that with
	//the current one devided by the Indentation you set

	SetScrollRange(hwnd, SB_VERT, 0, 100, FALSE);
	SetScrollPos(hwnd, SB_VERT, 100, TRUE);

}

void EnableShortCuts(HWND hwnd, BOOL enable) {
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF1), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF2), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF3), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF4), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF5), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF6), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF7), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF8), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF9), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF10), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF11), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF12), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_RETURNPAPER), enable);
}

static int CALLBACK TabSheet1Proc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		keypro_init(hwnd);
		CheckRadioButton(hwnd, IDC_BKSBS, IDC_BKSLEFT,
		IDC_BKSBS + config.bksp_is_delete);

		CheckRadioButton(hwnd, IDC_DELETERETURN, IDC_DELETEDEL,
				config.deleteisreturn == 1 ? IDC_DELETERETURN : IDC_DELETEDEL);
		if (config.allowshortcuts == 1) {
			EnableShortCuts(hwnd, TRUE);
			CheckDlgButton(hwnd, IDC_ALLOWSHORTCUTS, BST_CHECKED);
		} else if (config.allowshortcuts == 0) {
			CheckDlgButton(hwnd, IDC_ALLOWSHORTCUTS, BST_UNCHECKED);
			EnableShortCuts(hwnd, FALSE);
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BKSBS:
			config.bksp_is_delete = 0;
			break;
		case IDC_BKSDEL:
			config.bksp_is_delete = 1;
			break;
		case IDC_BKSLEFT:
			config.bksp_is_delete = 2;
			break;
		case IDC_DELETEDEL:
			config.deleteisreturn = 0;
			break;
		case IDC_DELETERETURN:
			config.deleteisreturn = 1;
			break;
		case IDC_ALLOWSHORTCUTS:
			if (IsDlgButtonChecked(hwnd, IDC_ALLOWSHORTCUTS) == BST_CHECKED) {
				config.allowshortcuts = 1;
				EnableShortCuts(hwnd, TRUE);

			} else {
				config.allowshortcuts = 0;
				EnableShortCuts(hwnd, FALSE);
			}
			break;
		}
		return 0;

	}
	return false;
}

static void colourtodialog(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_CURSONR, cursonR, false);
	SetDlgItemInt(hwnd, IDC_CURSONG, cursonG, false);
	SetDlgItemInt(hwnd, IDC_CURSONB, cursonB, false);
	SetDlgItemInt(hwnd, IDC_FONTR, fontR, false);
	SetDlgItemInt(hwnd, IDC_FONTG, fontG, false);
	SetDlgItemInt(hwnd, IDC_FONTB, fontB, false);
	SetDlgItemInt(hwnd, IDC_BACKR, backR, false);
	SetDlgItemInt(hwnd, IDC_BACKG, backG, false);
	SetDlgItemInt(hwnd, IDC_BACKB, backB, false);
	SetDlgItemInt(hwnd, IDC_EDGER, boderR, false);
	SetDlgItemInt(hwnd, IDC_EDGEG, boderG, false);
	SetDlgItemInt(hwnd, IDC_EDGEB, boderB, false);

}

void EnableCurrentUser(HWND hwnd, BOOL enable) {
	EnableWindow(GetDlgItem(hwnd, IDC_SYSPASS), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_SYSUSER), enable);
}

void ConfigDialog::InitTabSheet2(HWND hwnd) {
	cfg_tocolours();
	colourtodialog(hwnd);

	SetDlgItemTextA(hwnd, IDC_SYSUSER, config.sysuser);
	SetDlgItemTextA(hwnd, IDC_SYSPASS, config.syspass);
	if (config.currentuser == 1) {
		EnableCurrentUser(hwnd, false);
		CheckDlgButton(hwnd, IDC_CURRENTUSER, BST_CHECKED);
	} else {
		EnableCurrentUser(hwnd, true);
		CheckDlgButton(hwnd, IDC_CURRENTUSER, BST_UNCHECKED);
	}
	CheckDlgButton(hwnd, IDC_DEBUG, config.debug);
	SetDlgItemTextA(hwnd, IDC_UPDATEIP, config.updateip);

	SendMessageA(GetDlgItem(hwnd, IDC_ADMINPASSWORD), EM_LIMITTEXT, 6, 0);
	SetDlgItemTextA(hwnd, IDC_ADMINPASSWORD, config.adminpassword);
	SetDlgItemInt(hwnd, IDC_UPDATEPORT, config.updateport, false);
	CheckRadioButton(hwnd, IDC_CURSONTYPE0, IDC_CURSONTYPE2,
	IDC_CURSONTYPE0 + config.cursor_type);

	SetDlgItemInt(hwnd, IDC_FONTHEIGHT, config.font.height, false);
	SetDlgItemInt(hwnd, IDC_FONTWIDTH, config.fontwidth, false);

	CheckDlgButton(hwnd, IDC_NONUMLOCK, config.no_applic_k);
	if (config.beep == 1) {
		CheckRadioButton(hwnd, IDC_DEFAULT, IDC_WAVFILE, IDC_DEFAULT);
		EnableWindow(GetDlgItem(hwnd, IDC_FILEPATH), FALSE);
	} else if (config.beep == 3) {
		CheckRadioButton(hwnd, IDC_DEFAULT, IDC_WAVFILE, IDC_WAVFILE);
		SetDlgItemTextA(hwnd, IDC_FILEPATH, config.bell_wavefile.path);
	}
	CheckDlgButton(hwnd, IDC_TEXTBLINK, config.blinktext);
	CheckDlgButton(hwnd, IDC_LOGTYPE,
			config.logtype == 2 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_WARNONCLOSE, config.close_on_exit);
	CheckDlgButton(hwnd, IDC_AUTOCOPY, config.autocopy);
	CheckDlgButton(hwnd, IDC_CURSONBLINK, config.blink_cur);
	CheckDlgButton(hwnd, IDC_CHECKDISPLAY, config.cursor_on);
	CheckDlgButton(hwnd, IDC_ALLOWREPEAT, config.allowrepeat);
	SendMessage(GetDlgItem(hwnd, IDC_LINECODEPAGE), CB_ADDSTRING, 0,
			(LPARAM) TEXT("UTF-8"));
	SendMessage(GetDlgItem(hwnd, IDC_LINECODEPAGE), CB_ADDSTRING, 0,
			(LPARAM) TEXT("GBK"));

	SendMessage(GetDlgItem(hwnd, IDC_LINECODEPAGE), WM_SETTEXT, 0,
			(LPARAM) (config.line_codepage));

}

static int CALLBACK TabSheet2Proc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	COLORREF result;
	COLORREF g_rgbCustom[16]; //16个颜色存储，指定对话框的16个颜色选择
	CHOOSECOLOR cc;
	switch (msg) {
	case WM_INITDIALOG: {
		ConfigDialog::InitTabSheet2(hwnd);
		SetDlgItemTextA(hwnd, IDC_FILEPATH, config.bell_wavefile.path);
		return true;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DEBUG:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG) == BST_CHECKED) {
				config.debug = 1;
			} else {
				config.debug = 0;
			}
			break;
		case IDC_CURRENTUSER:
			if (IsDlgButtonChecked(hwnd, IDC_CURRENTUSER) == BST_CHECKED) {
				config.currentuser = 1;
				EnableCurrentUser(hwnd, false);
			} else {
				config.currentuser = 0;
				EnableCurrentUser(hwnd, true);
			}
			break;
		case IDC_CURSONTYPE0:
			config.cursor_type = 0;
			break;
		case IDC_CURSONTYPE1:
			config.cursor_type = 1;
			break;
		case IDC_CURSONTYPE2:
			config.cursor_type = 2;
			break;
		case IDC_CHECKDISPLAY:
			if (IsDlgButtonChecked(hwnd, IDC_CHECKDISPLAY) == BST_CHECKED)
				config.cursor_on = 1;
			else
				config.cursor_on = 0;
			break;
		case IDC_CURSONBLINK:
			if (IsDlgButtonChecked(hwnd, IDC_CURSONBLINK) == BST_CHECKED)
				config.blink_cur = 1;
			else
				config.blink_cur = 0;
			break;
		case IDC_DEFAULT:
			EnableWindow(GetDlgItem(hwnd, IDC_FILEPATH), FALSE);
			config.beep = 1;
			break;
		case IDC_WAVFILE:
			EnableWindow(GetDlgItem(hwnd, IDC_FILEPATH), TRUE);
			config.beep = 3;
			break;
		case IDC_TEXTBLINK:
			if (IsDlgButtonChecked(hwnd, IDC_TEXTBLINK) == BST_CHECKED)
				config.blinktext = 1;
			else
				config.blinktext = 0;
			break;
		case IDC_LOGTYPE:
			if (IsDlgButtonChecked(hwnd, IDC_LOGTYPE) == BST_CHECKED)
				config.logtype = 2;
			else
				config.logtype = 0;
			break;
		case IDC_WARNONCLOSE:
			if (IsDlgButtonChecked(hwnd, IDC_WARNONCLOSE) == BST_CHECKED)
				config.close_on_exit = 1;
			else
				config.close_on_exit = 0;
			break;
		case IDC_AUTOCOPY:
			if (IsDlgButtonChecked(hwnd, IDC_AUTOCOPY) == BST_CHECKED)
				config.autocopy = 1;
			else
				config.autocopy = 0;
			break;
		case IDC_NONUMLOCK:
			if (IsDlgButtonChecked(hwnd, IDC_NONUMLOCK) == BST_CHECKED)
				config.no_applic_k = 1;
			else
				config.no_applic_k = 0;
			break;
		case IDC_ALLOWREPEAT:
			if (IsDlgButtonChecked(hwnd, IDC_ALLOWREPEAT) == BST_CHECKED) {
				config.allowrepeat = 1;
//				MessageBoxA(hwnd, REPEAT, APPLICATION_NAME, MB_OK);
			} else
				config.allowrepeat = 0;
			break;
		case IDC_BUTTONBACKGROUND:
		case IDC_BUTTONFONT:
		case IDC_BUTTONEDGEC:
		case IDC_BUTTONCURSON:
			for (iCircle = 0; iCircle < 16; iCircle++) {
				g_rgbCustom[iCircle] = RGB(255, 255, 255); //全部初始化为白色
			}
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.rgbResult = RGB(0, 0, 0); //默认选中颜色
			cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR | CC_SOLIDCOLOR;
			cc.hwndOwner = hwnd;
			cc.lpCustColors = g_rgbCustom;
			if (ChooseColor(&cc)) {
				result = cc.rgbResult;

				int red, green, blue;
				red = GetRValue(result);
				green = GetGValue(result);
				blue = GetBValue(result);
				switch (LOWORD(wParam)) {
				case IDC_BUTTONCURSON:
					cursonR = red;
					cursonG = green;
					cursonB = blue;
					break;
				case IDC_BUTTONFONT:
					fontR = red;
					fontG = green;
					fontB = blue;
					break;
				case IDC_BUTTONBACKGROUND:
					backR = red;
					backG = green;
					backB = blue;
					break;
				case IDC_BUTTONEDGEC:
					boderR = red;
					boderG = green;
					boderB = blue;
					break;
				}

				colourtodialog(hwnd);
			}
			break;
		case IDC_BROWSE:
			if (Util::SelectFolder(hwnd, config.bell_wavefile.path,
			SELECTWAVFILE,
			BIF_BROWSEINCLUDEFILES))
				SetDlgItemTextA(hwnd, IDC_FILEPATH, config.bell_wavefile.path);
			break;
		}
		return 0;
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return 0;
	}
	return 0;
}

void InitTabSheet3(HWND hwnd) {
	SetDlgItemTextA(hwnd, IDC_EDITESC, config.keycodeesc);
	SetDlgItemTextA(hwnd, IDC_EDITINSERT, config.keycodeinsert);
	SetDlgItemTextA(hwnd, IDC_EDITDELETE, config.keycodedelete);
	SetDlgItemTextA(hwnd, IDC_EDITHOME, config.keycodehome);
	SetDlgItemTextA(hwnd, IDC_EDITNUMLOCK, config.keycodenumlock);
	SetDlgItemTextA(hwnd, IDC_EDITBACKSPACE, config.keycodespace);
	SetDlgItemTextA(hwnd, IDC_EDITPRINTSCREEN, config.keycodeprintscreen);
	SetDlgItemTextA(hwnd, IDC_EDITSCROLL, config.keycodescroll);
	SetDlgItemTextA(hwnd, IDC_EDITEND, config.keycodeend);
	SetDlgItemTextA(hwnd, IDC_EDITPAGEUP, config.keycodepageup);
	SetDlgItemTextA(hwnd, IDC_EDITPAGEDOWN, config.keycodepagedown);
	SetDlgItemTextA(hwnd, IDC_EDITADD, config.keycodeadd);
	SetDlgItemTextA(hwnd, IDC_EDITSUB, config.keycodesub);
	SetDlgItemTextA(hwnd, IDC_EDITDIV, config.keycodediv);
	SetDlgItemTextA(hwnd, IDC_EDITMULT, config.keycodemult);
	SetDlgItemTextA(hwnd, IDC_EDITPAUSE, config.keycodepause);
	SetDlgItemTextA(hwnd, IDC_EDITUP, config.keycodeup);
	SetDlgItemTextA(hwnd, IDC_EDITDOWN, config.keycodedown);
	SetDlgItemTextA(hwnd, IDC_EDITLEFT, config.keycodeleft);
	SetDlgItemTextA(hwnd, IDC_EDITRIGHT, config.keycoderight);
}

void EnableFunction(HWND hwnd, bool enable) {
	EnableWindow(GetDlgItem(hwnd, IDC_EDITESC), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITINSERT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITDELETE), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITHOME), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITBACKSPACE), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITNUMLOCK), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITPRINTSCREEN), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITSCROLL), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITEND), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITPAGEUP), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITPAGEDOWN), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITADD), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITSUB), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITDIV), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITMULT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITPAUSE), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITUP), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITDOWN), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITLEFT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITRIGHT), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_EDITF12), enable);
}
static int CALLBACK TabSheet3Proc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		InitTabSheet3(hwnd);
		if (config.allowfunction == 1) {
			CheckDlgButton(hwnd, IDC_ALLOWFUNCTION, BST_CHECKED);
			EnableFunction(hwnd, TRUE);
		} else if (config.allowfunction == 0) {
			CheckDlgButton(hwnd, IDC_ALLOWFUNCTION, BST_UNCHECKED);
			EnableFunction(hwnd, FALSE);
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ALLOWFUNCTION:
			if (IsDlgButtonChecked(hwnd, IDC_ALLOWFUNCTION) == BST_CHECKED) {
				config.allowfunction = 1;
				EnableFunction(hwnd, TRUE);
			} else {
				config.allowfunction = 0;
				EnableFunction(hwnd, FALSE);

			}
			break;
		}
		return 0;

	}
	return false;
}

int ConfigDialog::GetComSet(HWND hwnd, int ID, int add) {
	char ComValue[8];
	memset(ComValue, 0x00, 8);
	GetDlgItemText(hwnd, ID, ComValue, 8);
	return atoi(ComValue + add);
}

void ConfigDialog::SaveTabSheet(int iCurrentpage) {
	switch (iCurrentpage) {
	case 0:
		if (InputIPSuccess(g_hTabSheet[0], IDC_IPADDRESSVALUE, IDC_EDITPORT,
				&config) == 0)
			return;

		config.com01y = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM1Y, 3);
		config.com02z = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM2Z, 3);
		config.com03x = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM3X, 3);
		config.com04u = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM4U, 3);
		config.com05w = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM5W, 3);
		config.com06v = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM6V, 3);
		config.printcom = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_PRINTCOM,
				3);
		if (config.autologin == 1) {
			GetDlgItemText(g_hTabSheet[0], IDC_LOGINPROMPT, config.loginprompt,
					128);
			GetDlgItemText(g_hTabSheet[0], IDC_PASSWORDPROMPT,
					config.passwordprompt, 128);
			GetDlgItemText(g_hTabSheet[0], IDC_USERNAME, config.autousername,
			NAME_LEN);
			GetDlgItemText(g_hTabSheet[0], IDC_PASSWORD, config.autopassword,
			NAME_LEN);
		}
		config.ping_interval = GetDlgItemInt(g_hTabSheet[0], IDC_PINGINTERVAL,
		NULL, false);
		if (config.printmode == 0) {
			GetDlgItemTextA(g_hTabSheet[0], IDC_PRINTERNAME, config.printer,
			NAME_LEN);
		} else if (config.printmode == 3) {
			GetDlgItemTextA(g_hTabSheet[0], IDC_PRINTERNAME, config.fprintname,
			NAME_LEN);
			GetDlgItemTextA(g_hTabSheet[0], IDC_EPSHARE, config.fpshare,
			NAME_LEN);
		}
		GetDlgItemTextA(g_hTabSheet[0], IDC_SCREENNUM, config.screennum,
		NAME_LEN);

		if ((config.printmode == 0) && (!*config.printer)) {
			MessageBoxA(hwnd, PRINTNAME, APPLICATION_NAME, MB_OK);
			return;
		}

		break;
	case 1:
		dlg_to_cfg(g_hTabSheet[1]);
		terminal::ConfigDialog::set_cfgto_uchar();
		break;
	case 2:
		colours_tocfg1(g_hTabSheet[2]);
		config.font.height = GetDlgItemInt(g_hTabSheet[2], IDC_FONTHEIGHT, NULL,
				false);
		config.fontwidth = GetDlgItemInt(g_hTabSheet[2], IDC_FONTWIDTH, NULL,
				false);
		GetDlgItemText(g_hTabSheet[2], IDC_LINECODEPAGE, config.line_codepage,
				128);
		GetDlgItemText(g_hTabSheet[2], IDC_UPDATEIP, config.updateip, 32);
		config.updateport = GetDlgItemInt(g_hTabSheet[2], IDC_UPDATEPORT, NULL,
		FALSE);
		GetDlgItemText(g_hTabSheet[2], IDC_ADMINPASSWORD, config.adminpassword,
				128);
		GetDlgItemText(g_hTabSheet[2], IDC_SYSUSER, config.sysuser, 128);
		GetDlgItemText(g_hTabSheet[2], IDC_SYSPASS, config.syspass, 128);

		Util::KillProcess("update.exe");
		Sleep(1000);
		char exepath[MAX_PATH];
		sprintf(exepath, "%s\\update.exe", currentpath);
		if (Util::FileExists(exepath))
			Util::ShellExecuteNoWait(exepath);
		break;
	case 3:
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITESC, config.keycodeesc,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITINSERT, config.keycodeinsert,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITDELETE, config.keycodedelete,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITHOME, config.keycodehome,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITBACKSPACE, config.keycodespace,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITNUMLOCK, config.keycodenumlock,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPRINTSCREEN,
				config.keycodeprintscreen, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITSCROLL, config.keycodescroll,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITEND, config.keycodeend,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPAGEUP, config.keycodepageup,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPAGEDOWN,
				config.keycodepagedown,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITADD, config.keycodeadd,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITSUB, config.keycodesub,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITDIV, config.keycodediv,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITMULT, config.keycodemult,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPAUSE, config.keycodepause,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITUP, config.keycodeup, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITDOWN, config.keycodedown,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITLEFT, config.keycodeleft,
		KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITRIGHT, config.keycoderight,
		KEY_LEN);
		Update_KeyPairs();
		break;
	default:
		break;
	}

	Save();
}
void InitTabControl(HWND hwnd) {
	TCITEM tie;
	TCHAR szTitle[30];
	RECT rt;

	tie.mask = TCIF_TEXT;
	tie.pszText = szTitle;

	snprintf(szTitle, 29, TABITEM_BASIC);
	TabCtrl_InsertItem(hwnd, 0, &tie);
	snprintf(szTitle, 29, TABITEM_DEFINEKEY);
	TabCtrl_InsertItem(hwnd, 1, &tie);
	snprintf(szTitle, 29, TABITEM_COLOUR);
	TabCtrl_InsertItem(hwnd, 3, &tie);
	snprintf(szTitle, 29, TABITEM_FUNCTION);
	TabCtrl_InsertItem(hwnd, 3, &tie);

	g_hTabSheet[0] = CreateDialogA(hinst, (LPCTSTR)IDD_DIALOG_SHEET1, hwnd,
			(DLGPROC )TabSheet0Proc);
	g_hTabSheet[1] =
	CreateDialog(hinst,(LPCTSTR)IDD_DIALOG_SHEET2,hwnd,(DLGPROC)TabSheet1Proc);
	g_hTabSheet[2] =
	CreateDialog(hinst,(LPCTSTR)IDD_DIALOG_SHEET3,hwnd,(DLGPROC)TabSheet2Proc);
	g_hTabSheet[3] =
	CreateDialog(hinst,(LPCTSTR)IDD_DIALOG_SHEET4,hwnd,(DLGPROC)TabSheet3Proc);
	GetClientRect(hwnd, &rt);
	TabCtrl_AdjustRect(hwnd, false, &rt);
	for (int i = 0; i < TABSHEETMAX; i++) {
		MoveWindow(g_hTabSheet[i], rt.left, rt.top, rt.right - rt.left,
				rt.bottom - rt.top, false);
	}

}

static int CALLBACK SetProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	static int iCurrentPage;

	HWND g_hwndTab = GetDlgItem(hwnd, IDC_TAB1);
	switch (msg) {
	case WM_INITDIALOG:
		MoveToCenter(hwnd);
		InitTabControl(g_hwndTab);
		iCurrentPage = 0;
		TabCtrl_SetCurSel(g_hwndTab, iCurrentPage);
		ShowWindow(g_hTabSheet[iCurrentPage], true);
		return 1;
	case WM_NOTIFY:
		if ((int) wParam == IDC_TAB1) {
			if (((LPNMHDR) lParam)->code == TCN_SELCHANGE) {
				iCurrentPage = TabCtrl_GetCurSel(g_hwndTab);
				for (int i = 0; i < TABSHEETMAX; i++) {
					ShowWindow(g_hTabSheet[i], false);
				}
				ShowWindow(g_hTabSheet[iCurrentPage], true);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_APPLICATION:
			iCurrentPage = TabCtrl_GetCurSel(g_hwndTab);
			ConfigDialog::SaveTabSheet(iCurrentPage);
			break;
		case IDOK: {
			ConfigDialog::SaveTabSheet(0);
			ConfigDialog::SaveTabSheet(1);
			ConfigDialog::SaveTabSheet(2);
			ConfigDialog::SaveTabSheet(3);
			EndDialog(hwnd, TRUE);
		}
			return 0;
		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			for (int i = 0; i < TABSHEETMAX; i++) {
				EndDialog(g_hTabSheet[i], true);
			}
			break;
		case IDC_RESUMVALUE:
			if (iCurrentPage == 1) {
				keypro_resumvalue();
				keypro_init(g_hTabSheet[1]);
			} else if (iCurrentPage == 2) {
				colours_resumvalue();
				colours_tocfg1(g_hTabSheet[2]);
				colourtodialog(g_hTabSheet[2]);
			}
			Save();
			break;
		}
		return 0;

	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return 0;
	}
	return 0;
}

void ConfigDialog::ShowSet(HWND hwnd) {
	DialogBox(hinst,MAKEINTRESOURCE(IDD_SET),hwnd,SetProc);
}

static int CALLBACK AdminProc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	char password[50];
	switch (msg) {
	case WM_INITDIALOG:
		MoveToCenter(hwnd);
		memset(password, 0x00, 50);
		SendMessageA(GetDlgItem(hwnd, IDC_PASSWORD), EM_LIMITTEXT, 6, 0);
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemTextA(hwnd, IDC_PASSWORD, password, 7);
			if (memcmp(password, config.adminpassword, 6) == 0) {
				config.admin = 1;
			} else {
				config.admin = 0;
				if (MessageBoxA(hwnd, INPUTPASSWORD, APPLICATION_NAME,
				MB_OKCANCEL) == IDOK) {
					SetFocus(GetDlgItem(hwnd, IDC_PASSWORD));
					return 0;
				}
			}
			EndDialog(hwnd, TRUE);
			return 0;
			break;
		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			return 0;
		}
		return 0;

	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return 0;
	}
	return 0;
}

void ConfigDialog::ShowAdmin(HWND hwnd) {
	DialogBox(hinst,MAKEINTRESOURCE(IDD_ADMIN),hwnd,AdminProc);
}

static int CALLBACK NewTermProc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {

	char szPath[MAX_PATH];
	char termname[512];
	char section[24];

	char description[1024];
	char iptemp[512];
	int porttemp;
	char screennumtemp[128];
	memcpy(iptemp, config.host, 512);
	porttemp = config.port;
	switch (msg) {
	case WM_INITDIALOG: {
		MoveToCenter(hwnd);
		strcpy(szPath, config.linkpath);
		if (strcmp(szPath, "") == 0) {
			Util::GetDesktopPath(szPath);
			CheckRadioButton(hwnd, IDC_SELECT, IDC_DESKTOP, IDC_DESKTOP);
			EnableWindow(GetDlgItem(hwnd, IDC_SHORTCUTSPATH), FALSE);
		} else {
			CheckRadioButton(hwnd, IDC_SELECT, IDC_DESKTOP, IDC_SELECT);
			EnableWindow(GetDlgItem(hwnd, IDC_SHORTCUTSPATH), TRUE);
		}
		SetDlgItemTextA(hwnd, IDC_SHORTCUTSPATH, szPath);
	}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SELECT:
			EnableWindow(GetDlgItem(hwnd, IDC_SHORTCUTSPATH), TRUE);
			break;
		case IDC_BROWSE:
			if (Util::SelectFolder(hwnd, szPath, SELECTWAVFILE,
			BIF_NEWDIALOGSTYLE)) { //BIF_RETURNONLYFSDIRS))
				SetDlgItemTextA(hwnd, IDC_SHORTCUTSPATH, szPath);
			}
			break;
		case IDOK:
			char icopath[MAX_PATH];

			memset(termname, 0, 512);
			GetDlgItemTextA(hwnd, IDC_TERMNAME, termname, 512);
			if (termname[0] == 0) {
				MessageBoxA(hwnd, INPUTTERMNAME, TITLE_POINT, MB_OK);
				return 0;
			}

			GetDlgItemTextA(hwnd, IDC_SHORTCUTSPATH, szPath, sizeof(szPath));
			sprintf(szPath, "%s\\%s.lnk", szPath, termname);
			GetDlgItemTextA(hwnd, IDC_DESCRIPTION, description,
					sizeof(description));

			GetDlgItemTextA(hwnd, IDC_SECTIONNAME, section, sizeof(section));
			sprintf(icopath, "%s\\puttycfg.ico", currentpath);

			if (InputIPSuccess(hwnd, IDC_TERMIP, IDC_TERMPORT, &config) == 0)
				return 0;
			CoInitialize(NULL);
			memset(screennumtemp, 0x00, 128);
			memcpy(screennumtemp, config.screennum, 128);
			char exepath[128];
			sprintf(exepath, "%s\\shell.exe", currentpath);
			if (CreateShortCut(exepath, section, szPath, description,
			SW_SHOWNORMAL, currentpath, icopath, 0) == 0) {
				GetDlgItemTextA(hwnd, IDC_NEWSCREENNUM, config.screennum, 128);
				MessageBoxA(hwnd, NEWTERMSUCCESS, TITLE_POINT, MB_OK);
				Save();
			}
			memcpy(config.host, iptemp, 512);
			config.port = porttemp;
			memset(config.screennum, 0x00, 128);
			memcpy(config.screennum, screennumtemp, 128);
			Save();
			return 0;
		case IDCANCEL:
			EndDialog(hwnd, TRUE);
			return 0;
		}
		break;
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return 0;

	}
	return 0;
}

void ConfigDialog::ShowNewTerm(HWND hwnd) {
	DialogBox(hinst, MAKEINTRESOURCE(IDD_ADDTERMINAL), hwnd, NewTermProc);
}
static int CALLBACK HelpProc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		MoveToCenter(hwnd);
		break;
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return 0;
	}
	return 0;
}

void ConfigDialog::ShowHelp(HWND hwnd) {
	DialogBox(hinst,MAKEINTRESOURCE(IDD_HELP),hwnd,HelpProc);
}

}
