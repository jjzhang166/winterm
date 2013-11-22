/*
 * ConfigDialog.cpp
 *
 *  Created on: 2012-9-25
 *      Author: xia
 */
#define   NO_WIN32_LEAN_AND_MEAN

#include "ConfigDialog.h"
#include <stdio.h>
#include "cpputils/Util.h"
#include "util/windows/shortcut.h"
#include <iostream>
#include "storage.h"
#include "Setting.h"

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
extern int IsNewConnection;
namespace terminal {

ConfigDialog::ConfigDialog() {
	// TODO Auto-generated constructor stub

}

ConfigDialog::~ConfigDialog() {
	// TODO Auto-generated destructor stub
}

//void colours_tocfg() {
//	cfg.colours[0][0] = fontR;
//	cfg.colours[0][1] = fontG;
//	cfg.colours[0][2] = fontB;
//	cfg.colours[1][0] = boderR;
//	cfg.colours[1][1] = boderG;
//	cfg.colours[1][2] = boderB;
//	cfg.colours[2][0] = backR;
//	cfg.colours[2][1] = backG;
//	cfg.colours[2][2] = backB;
//	cfg.colours[5][0] = cursonR;
//	cfg.colours[5][1] = cursonG;
//	cfg.colours[5][2] = cursonB;
//
//}
void colours_tocfg1(HWND hwnd) {
	cfg.colours[0][0] = GetDlgItemInt(hwnd, IDC_FONTR, NULL, FALSE);
	cfg.colours[0][1] = GetDlgItemInt(hwnd, IDC_FONTG, NULL, FALSE);
	cfg.colours[0][2] = GetDlgItemInt(hwnd, IDC_FONTB, NULL, FALSE);
	cfg.colours[1][0] = GetDlgItemInt(hwnd, IDC_EDGER, NULL, FALSE);
	cfg.colours[1][1] = GetDlgItemInt(hwnd, IDC_EDGEG, NULL, FALSE);
	cfg.colours[1][2] = GetDlgItemInt(hwnd, IDC_EDGEB, NULL, FALSE);
	cfg.colours[2][0] = GetDlgItemInt(hwnd, IDC_BACKR, NULL, FALSE);
	cfg.colours[2][1] = GetDlgItemInt(hwnd, IDC_BACKG, NULL, FALSE);
	cfg.colours[2][2] = GetDlgItemInt(hwnd, IDC_BACKB, NULL, FALSE);
	cfg.colours[5][0] = GetDlgItemInt(hwnd, IDC_CURSONR, NULL, FALSE);
	cfg.colours[5][1] = GetDlgItemInt(hwnd, IDC_CURSONG, NULL, FALSE);
	cfg.colours[5][2] = GetDlgItemInt(hwnd, IDC_CURSONB, NULL, FALSE);
}

void cfg_tocolours() {
	cursonR = cfg.colours[5][0];
	cursonG = cfg.colours[5][1];
	cursonB = cfg.colours[5][2];

	fontR = cfg.colours[0][0];
	fontG = cfg.colours[0][1];
	fontB = cfg.colours[0][2];

	backR = cfg.colours[2][0];
	backG = cfg.colours[2][1];
	backB = cfg.colours[2][2];

	boderR = cfg.colours[1][0];
	boderG = cfg.colours[1][1];
	boderB = cfg.colours[1][2];

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

static int CALLBACK RegProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	char InputKey[128];

	switch (msg) {
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		CloseHandle(hMutex);
		exit(1);
	case WM_INITDIALOG:
		SetDlgItemText(hwnd, IDC_STATICMACHCODE, cfg.registcod);
		SetWindowTextA(hwnd, APPLICATION_NAME);
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_MACHCODE:
			if (OpenClipboard(hwnd)) {
				HGLOBAL hmem = GlobalAlloc(GMEM_DDESHARE, 256);
				LPVOID pmem = GlobalLock(hmem);
				EmptyClipboard();
				memcpy(pmem, cfg.registcod, strlen(cfg.registcod));
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
			strcpy(cfg.registkey, localRegistKey);
			Setting::save_reg_config();
//			Setting::save_settings(sectionName, &cfg);
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
	char severHost[32];

	memset(textIPValue, 0x00, 32);
	GetDlgItemText(hwnd, IPID, textIPValue, 32);
	textPortValue = GetDlgItemInt(hwnd, PORTID, NULL, false);
	memset(severHost, 0x00, 32);
	strcpy(severHost, "0.0.0.0");
//	memcpy(severHost, "0.0.0.0", 7);
	if (memcmp(severHost, textIPValue, sizeof(textIPValue)) == 0) {
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

//static int CALLBACK ConnectionProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//
//	switch (msg) {
//	case WM_INITDIALOG:
//		SetDlgItemText(hwnd, IDC_IPADDRESS, cfg.host);
//		SetDlgItemInt(hwnd, IDC_EDITPORT, cfg.port, false);
//		return 1;
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//
//		return 0;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//
//		case IDCANCEL:
//			if (InputIPSuccess(hwnd, IDC_IPADDRESS, IDC_EDITPORT, &cfg) != 0) {
//				MessageBoxA(hwnd, "set ip address success", "wizool", MB_OK);
//				Setting::save_settings(sectionName, &cfg);
//				EndDialog(hwnd, TRUE);
//			}
//			break;
//		case IDC_DEL:
//			memset(cfg.host, 0x00, 32);
//			cfg.port = 0;
//			SetDlgItemText(hwnd, IDC_IPADDRESS, "");
//			SetDlgItemText(hwnd, IDC_EDITPORT, "");
//			Setting::save_settings(sectionName, &cfg);
//			break;
//
//		}
//		return 0;
//	}
//	return 0;
//}
//void ConfigDialog::ShowConnection(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_CONNECTION), hwnd, ConnectionProc);
//}
void keypro_resumvalue() {

	strcpy(cfg.keycodef[1], "^[OP");
	strcpy(cfg.keycodef[2], "^[OQ");
	strcpy(cfg.keycodef[3], "^[OR");
	strcpy(cfg.keycodef[4], "^[OS");
	strcpy(cfg.keycodef[5], "^[Ot");
	strcpy(cfg.keycodef[6], "^[Ou");
	strcpy(cfg.keycodef[7], "^[Ov");
	strcpy(cfg.keycodef[8], "^[Ol");
	strcpy(cfg.keycodef[9], "^[Ow");
	strcpy(cfg.keycodef[10], "^[Ox");
	strcpy(cfg.keycodef[11], "^[Oy");
	strcpy(cfg.keycodef[12], "");
}

void keypro_init(HWND hwnd) {
	SetDlgItemText(hwnd, IDC_EDITF1, cfg.keycodef[1]);
	SetDlgItemText(hwnd, IDC_EDITF2, cfg.keycodef[2]);
	SetDlgItemText(hwnd, IDC_EDITF3, cfg.keycodef[3]);
	SetDlgItemText(hwnd, IDC_EDITF4, cfg.keycodef[4]);
	SetDlgItemText(hwnd, IDC_EDITF5, cfg.keycodef[5]);
	SetDlgItemText(hwnd, IDC_EDITF6, cfg.keycodef[6]);
	SetDlgItemText(hwnd, IDC_EDITF7, cfg.keycodef[7]);
	SetDlgItemText(hwnd, IDC_EDITF8, cfg.keycodef[8]);
	SetDlgItemText(hwnd, IDC_EDITF9, cfg.keycodef[9]);
	SetDlgItemText(hwnd, IDC_EDITF10, cfg.keycodef[10]);
	SetDlgItemText(hwnd, IDC_EDITF11, cfg.keycodef[11]);
	SetDlgItemText(hwnd, IDC_EDITF12, cfg.keycodef[12]);
}

void dlg_to_cfg(HWND hwnd) {
	GetDlgItemText(hwnd, IDC_EDITF1, cfg.keycodef[1], 1024);
	GetDlgItemText(hwnd, IDC_EDITF2, cfg.keycodef[2], 1024);
	GetDlgItemText(hwnd, IDC_EDITF3, cfg.keycodef[3], 1024);
	GetDlgItemText(hwnd, IDC_EDITF4, cfg.keycodef[4], 1024);
	GetDlgItemText(hwnd, IDC_EDITF5, cfg.keycodef[5], 1024);
	GetDlgItemText(hwnd, IDC_EDITF6, cfg.keycodef[6], 1024);
	GetDlgItemText(hwnd, IDC_EDITF7, cfg.keycodef[7], 1024);
	GetDlgItemText(hwnd, IDC_EDITF8, cfg.keycodef[8], 1024);
	GetDlgItemText(hwnd, IDC_EDITF9, cfg.keycodef[9], 1024);
	GetDlgItemText(hwnd, IDC_EDITF10, cfg.keycodef[10], 1024);
	GetDlgItemText(hwnd, IDC_EDITF11, cfg.keycodef[11], 1024);
	GetDlgItemText(hwnd, IDC_EDITF12, cfg.keycodef[12], 1024);
}
void ConfigDialog::set_cfgto_uchar() {
	for (int i = 1; i < KEYCODEFXX_MAX_NUM; i++) {
		KeyCodeLenFXX[i] = 0;
		for (int j = 0; j < strlen(cfg.keycodef[i]); j++) {

			if ((cfg.keycodef[i][j] == 0x5E)
					&& (cfg.keycodef[i][j + 1]) == 0x5B) {
				j++;
				KeyCodeFXX[i - 1][KeyCodeLenFXX[i - 1]++] = 0X1B;
			} else
				KeyCodeFXX[i - 1][KeyCodeLenFXX[i - 1]++] = cfg.keycodef[i][j];
		}

	}

}
//static int CALLBACK UserKeyProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//	char keyValue[8];
//	int rv = 0;
//	switch (msg) {
//	case WM_INITDIALOG:
//		keypro_init(hwnd);
//		return 1;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDC_RESUMVALUE:
//			if (MessageBox(hwnd, SURE_REVERT_DEFAULT, TITLE_POINT,
//					MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK) {
//				keypro_resumvalue();
//				keypro_init(hwnd);
//			}
//			break;
//		case IDOK:
//			dlg_to_cfg(hwnd);
//			Setting::save_settings(sectionName, &cfg);
//			terminal::ConfigDialog::set_cfgto_uchar();
//			MessageBox(hwnd, SAVE_SET_CUSTOM, TITLE_POINT,
//					MB_ICONINFORMATION | MB_OK | MB_DEFBUTTON1);
//			SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS), CB_SETCURSEL, -1, 0);
//			memset(keyValue, 0x00, 8);
//			sprintf(keyValue, "%d", cfg.port);
//			SetDlgItemText(hwnd, IDC_EDITPORT, keyValue);
//		case IDCANCEL:
//			EndDialog(hwnd, TRUE);
//			break;
//		}
//		return 0;
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	}
//	return 0;
//}
//
//void ConfigDialog::ShowUserKey(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_USERKEY), hwnd, UserKeyProc);
//}

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
	ComSetText(hwnd, cfg.com01y, IDC_COM1Y);
	ComSetText(hwnd, cfg.com02z, IDC_COM2Z);
	ComSetText(hwnd, cfg.com03x, IDC_COM3X);
	ComSetText(hwnd, cfg.com04u, IDC_COM4U);
	ComSetText(hwnd, cfg.com05w, IDC_COM5W);
	ComSetText(hwnd, cfg.com06v, IDC_COM6V);
	ComSetText(hwnd, cfg.printcom, IDC_PRINTCOM);

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
	if (cfg.allowrepeat == 1) {
		SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
				(LPARAM) SERVICEPRINT);

	} else if (cfg.allowrepeat == 0) {
		SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
				(LPARAM) DIRECTPRINT);

	}
	SendDlgItemMessage(hwnd, IDC_PRINTMODE, CB_ADDSTRING, 0,
			(LPARAM) FILEPRINT);
	if (cfg.printmode == 3) {
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(FILEPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, FILEPDIR);
		SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, FILENAME);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_SSHARE), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_EPSHARE), SW_SHOW);
		SetDlgItemTextA(hwnd, IDC_PRINTERNAME, cfg.fprintname);
		SetDlgItemTextA(hwnd, IDC_EPSHARE, cfg.fpshare);
	} else if ((cfg.printmode == 2) && (cfg.allowrepeat == 1)) {
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(SERVICEPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, SERVICEPDIR);
	} else if ((cfg.printmode == 2) && (cfg.allowrepeat == 0)) {
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(DIRECTPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, DIRECTPDIR);
//		ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_HIDE);
	} else if (cfg.printmode == 1) {
		SetDlgItemTextA(hwnd, IDC_SPDIR, SERIALPDIR);
		SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(SERIALPRINT));
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_SHOW);
//		ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_HIDE);
		SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTCOM);
	} else if (cfg.printmode == 0) {
		SendMessageA(GetDlgItem(hwnd, IDC_PRINTMODE), WM_SETTEXT, 0,
				(LPARAM) TEXT(DRIVEPRINT));
		SetDlgItemTextA(hwnd, IDC_SPDIR, DRIVEPRINT);
		SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTERNAME);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
//		ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_HIDE);
		SetDlgItemTextA(hwnd, IDC_PRINTERNAME, cfg.printer);
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
	SetDlgItemTextA(hwnd, IDC_SCOM1Y, cfg.comname01y);
	SetDlgItemTextA(hwnd, IDC_SCOM2Z, cfg.comname02z);
	SetDlgItemTextA(hwnd, IDC_SCOM3X, cfg.comname03x);
	SetDlgItemTextA(hwnd, IDC_SCOM4U, cfg.comname04u);
	SetDlgItemTextA(hwnd, IDC_SCOM5W, cfg.comname05w);
	SetDlgItemTextA(hwnd, IDC_SCOM6V, cfg.comname06v);
}

void ConfigDialog::InitTabSheet0(HWND hwnd) {

	SetDlgItemText(hwnd, IDC_IPADDRESSVALUE, cfg.host);
	SetDlgItemInt(hwnd, IDC_EDITPORT, cfg.port, false);
	SetDlgItemText(hwnd, IDC_SCREENNUM, cfg.screennum);
	CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO2,
			cfg.height == 24 ? IDC_RADIO1 : IDC_RADIO2);
	CheckRadioButton(hwnd, IDC_RADIO3, IDC_RADIO4,
			cfg.width == 80 ? IDC_RADIO3 : IDC_RADIO4);
	if (cfg.autologin == 1) {
		CheckDlgButton(hwnd, IDC_AUTOLOGIN, BST_CHECKED);
		SetDlgItemText(hwnd, IDC_LOGINPROMPT, cfg.loginprompt);
		SetDlgItemText(hwnd, IDC_PASSWORDPROMPT, cfg.passwordprompt);
		SetDlgItemText(hwnd, IDC_USERNAME, cfg.autousername);
		SetDlgItemText(hwnd, IDC_PASSWORD, cfg.autopassword);

	} else {
		CheckDlgButton(hwnd, IDC_AUTOLOGIN, BST_UNCHECKED);
		EnableDlgAutoLogin(hwnd, FALSE);
	}
	InitComCombox(hwnd);
	ShowComName(hwnd);
	ConfigDialog::InitCom(hwnd);
	InitPrintCombox(hwnd);
	CheckDlgButton(hwnd, IDC_TCPKEEPALIVES, cfg.tcp_keepalives);
	SetDlgItemInt(hwnd, IDC_PINGINTERVAL, cfg.ping_interval, false);
}

static void InitComName(HWND hwnd) {
	SetDlgItemTextA(hwnd, IDC_ECOM1, cfg.comname01y);
	SetDlgItemTextA(hwnd, IDC_ECOM2, cfg.comname02z);
	SetDlgItemTextA(hwnd, IDC_ECOM3, cfg.comname03x);
	SetDlgItemTextA(hwnd, IDC_ECOM4, cfg.comname04u);
	SetDlgItemTextA(hwnd, IDC_ECOM5, cfg.comname05w);
	SetDlgItemTextA(hwnd, IDC_ECOM6, cfg.comname06v);
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
	SetDlgItemInt(hwnd, IDC_BANDY, cfg.bandy, false);
	InitBandRateCom(hwnd, IDC_BANDY);
	InitByteCom(hwnd, IDC_BYTEY);
	InitStopCom(hwnd, IDC_STOPY);
	InitPariyCom(hwnd, IDC_PARIYY);
	InitUseSet(hwnd, IDC_USESETY, cfg.useselfsety);
}

static void InitComSetZ(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDZ, cfg.bandz, false);
	InitBandRateCom(hwnd, IDC_BANDZ);
	InitByteCom(hwnd, IDC_BYTEZ);
	InitStopCom(hwnd, IDC_STOPZ);
	InitPariyCom(hwnd, IDC_PARIYZ);
	InitUseSet(hwnd, IDC_USESETZ, cfg.useselfsetz);
}
static void InitComSetX(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDX, cfg.bandx, false);
	InitBandRateCom(hwnd, IDC_BANDX);
	InitByteCom(hwnd, IDC_BYTEX);
	InitStopCom(hwnd, IDC_STOPX);
	InitPariyCom(hwnd, IDC_PARIYX);
	InitUseSet(hwnd, IDC_USESETX, cfg.useselfsetx);
}
static void InitComSetU(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDU, cfg.bandu, false);
	InitBandRateCom(hwnd, IDC_BANDU);
	InitByteCom(hwnd, IDC_BYTEU);
	InitStopCom(hwnd, IDC_STOPU);
	InitPariyCom(hwnd, IDC_PARIYU);
	InitUseSet(hwnd, IDC_USESETU, cfg.useselfsetu);
}
static void InitComSetW(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDW, cfg.bandw, false);
	InitBandRateCom(hwnd, IDC_BANDW);
	InitByteCom(hwnd, IDC_BYTEW);
	InitStopCom(hwnd, IDC_STOPW);
	InitPariyCom(hwnd, IDC_PARIYW);
	InitUseSet(hwnd, IDC_USESETW, cfg.useselfsetw);
}
static void InitComSetV(HWND hwnd) {
	SetDlgItemInt(hwnd, IDC_BANDV, cfg.bandv, false);
	InitBandRateCom(hwnd, IDC_BANDV);
	InitByteCom(hwnd, IDC_BYTEV);
	InitStopCom(hwnd, IDC_STOPV);
	InitPariyCom(hwnd, IDC_PARIYV);
	InitUseSet(hwnd, IDC_USESETV, cfg.useselfsetv);
}
static void InitComSet(HWND hwnd) {
	InitComSetY(hwnd);
	InitComSetZ(hwnd);
	InitComSetX(hwnd);
	InitComSetU(hwnd);
	InitComSetW(hwnd);
	InitComSetV(hwnd);
}

//static void InitUseSet(HWND hwnd) {
//	InitUseSet(hwnd, IDC_USESETY, cfg.useselfsety);
//	InitUseSet(hwnd, IDC_USESETZ, cfg.useselfsetz);
//	InitUseSet(hwnd, IDC_USESETX, cfg.useselfsetx);
//	InitUseSet(hwnd, IDC_USESETU, cfg.useselfsetu);
//	InitUseSet(hwnd, IDC_USESETW, cfg.useselfsetw);
//	InitUseSet(hwnd, IDC_USESETV, cfg.useselfsetv);
//}

//
//static void InitBandRate(HWND hwnd) {
//	SetDlgItemInt(hwnd, IDC_BANDY, cfg.bandy, false);
//	SetDlgItemInt(hwnd, IDC_BANDZ, cfg.bandz, false);
//	SetDlgItemInt(hwnd, IDC_BANDX, cfg.bandx, false);
//	SetDlgItemInt(hwnd, IDC_BANDU, cfg.bandu, false);
//	SetDlgItemInt(hwnd, IDC_BANDW, cfg.bandw, false);
//	SetDlgItemInt(hwnd, IDC_BANDV, cfg.bandv, false);
//	InitBandRateCom(hwnd, IDC_BANDY);
//	InitBandRateCom(hwnd, IDC_BANDZ);
//	InitBandRateCom(hwnd, IDC_BANDX);
//	InitBandRateCom(hwnd, IDC_BANDU);
//	InitBandRateCom(hwnd, IDC_BANDW);
//	InitBandRateCom(hwnd, IDC_BANDV);
//}

static void ResetComName(HWND hwnd) {
	SetDlgItemTextA(hwnd, IDC_ECOM1, COMNAME1);
	SetDlgItemTextA(hwnd, IDC_ECOM2, COMNAME2);
	SetDlgItemTextA(hwnd, IDC_ECOM3, COMNAME3);
	SetDlgItemTextA(hwnd, IDC_ECOM4, COMNAME4);
	SetDlgItemTextA(hwnd, IDC_ECOM5, COMNAME5);
	SetDlgItemTextA(hwnd, IDC_ECOM6, COMNAME6);
}

static void SaveComName(HWND hwnd) {
	GetDlgItemTextA(hwnd, IDC_ECOM1, cfg.comname01y, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM2, cfg.comname02z, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM3, cfg.comname03x, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM4, cfg.comname04u, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM5, cfg.comname05w, NAME_LEN);
	GetDlgItemTextA(hwnd, IDC_ECOM6, cfg.comname06v, NAME_LEN);
}

static void SaveBandRate(HWND hwnd) {
	cfg.bandy = GetDlgItemInt(hwnd, IDC_BANDY, NULL, false);
	cfg.bandz = GetDlgItemInt(hwnd, IDC_BANDZ, NULL, false);
	cfg.bandx = GetDlgItemInt(hwnd, IDC_BANDX, NULL, false);
	cfg.bandu = GetDlgItemInt(hwnd, IDC_BANDU, NULL, false);
	cfg.bandw = GetDlgItemInt(hwnd, IDC_BANDW, NULL, false);
	cfg.bandv = GetDlgItemInt(hwnd, IDC_BANDV, NULL, false);
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
				cfg.useselfsety = 1;

			} else {
				cfg.useselfsety = 0;
			}
			break;
		case IDC_USESETZ:
			if (IsDlgButtonChecked(hwnd, IDC_USESETZ) == BST_CHECKED) {
				cfg.useselfsetz = 1;

			} else {
				cfg.useselfsetz = 0;
			}
			break;
		case IDC_USESETX:
			if (IsDlgButtonChecked(hwnd, IDC_USESETX) == BST_CHECKED) {
				cfg.useselfsetx = 1;

			} else {
				cfg.useselfsetx = 0;
			}
			break;
		case IDC_USESETU:
			if (IsDlgButtonChecked(hwnd, IDC_USESETU) == BST_CHECKED) {
				cfg.useselfsetu = 1;

			} else {
				cfg.useselfsetu = 0;
			}
			break;
		case IDC_USESETW:
			if (IsDlgButtonChecked(hwnd, IDC_USESETW) == BST_CHECKED) {
				cfg.useselfsetw = 1;

			} else {
				cfg.useselfsetw = 0;
			}
			break;
		case IDC_USESETV:
			if (IsDlgButtonChecked(hwnd, IDC_USESETV) == BST_CHECKED) {
				cfg.useselfsetv = 1;

			} else {
				cfg.useselfsetv = 0;
			}
			break;
		}
		break;
	}
	Setting::save_settings(sectionName, &cfg);
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
				cfg.printmode = SendMessage(GetDlgItem(hwnd, IDC_PRINTMODE),
						CB_GETCURSEL, 0, 0);
				EnableDlgPrinter(hwnd, SW_HIDE);
				if (cfg.printmode == 0) { //选择驱动打印
					SetDlgItemTextA(hwnd, IDC_SPDIR, DRIVEPDIR);
					SetDlgItemTextA(hwnd, IDC_PRINTERNAME, cfg.printer);
					SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTERNAME);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
				} else if (cfg.printmode == 1) {
					SetDlgItemTextA(hwnd, IDC_SPDIR, SERIALPDIR);
					SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, PRINTCOM);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTCOM), SW_SHOW);
				} else if (cfg.printmode == 2) {
					if (cfg.allowrepeat == 1) {
						SetDlgItemTextA(hwnd, IDC_SPDIR, SERVICEPDIR);
					} else if (cfg.allowrepeat == 0) {
						SetDlgItemTextA(hwnd, IDC_SPDIR, DIRECTPDIR);
					}
				} else if (cfg.printmode == 3) {
					SetDlgItemTextA(hwnd, IDC_SPDIR, FILEPDIR);
					SetDlgItemTextA(hwnd, IDC_PRINTERNAME, cfg.fprintname);
					SetDlgItemTextA(hwnd, IDC_EPSHARE, cfg.fpshare);
					SetDlgItemTextA(hwnd, IDC_PRINTSTATIC, FILENAME);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTSTATIC), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_PRINTERNAME), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_SSHARE), SW_SHOW);
					ShowWindow(GetDlgItem(hwnd, IDC_EPSHARE), SW_SHOW);

				}
			}
			break;

		case IDC_RADIO1:
			cfg.height = 24;
			break;
		case IDC_RADIO2:
			cfg.height = 25;
			break;
		case IDC_RADIO3:
			cfg.width = 80;
			break;
		case IDC_RADIO4:
			cfg.width = 132;
			break;
		case IDC_TCPKEEPALIVES:
			if (IsDlgButtonChecked(hwnd, IDC_TCPKEEPALIVES) == BST_CHECKED) {
				cfg.tcp_keepalives = 1;
			} else {
				cfg.tcp_keepalives = 0;
			}
			break;
		case IDC_AUTOLOGIN:
			if (IsDlgButtonChecked(hwnd, IDC_AUTOLOGIN) == BST_CHECKED) {
				cfg.autologin = 1;
				EnableDlgAutoLogin(hwnd, TRUE);
			} else {
				cfg.autologin = 0;
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

//void InitReturnPaperbox(HWND hwnd) {
//	char buf[32];
//	for (int i = 1; i < 12; i++) {
//		sprintf(buf, "F%d", i);
//		SendDlgItemMessage(hwnd, IDC_RETURNPAPER, CB_ADDSTRING, 0,
//				(LPARAM) buf);
//		memset(buf, 0x00, 32);
//	}
//	sprintf(buf, "F%d", cfg.returnpaper);
//	SendMessage(GetDlgItem(hwnd, IDC_RETURNPAPER), WM_SETTEXT, 0, (LPARAM) buf);
//
//}

static int CALLBACK TabSheet1Proc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	switch (msg) {
	case WM_INITDIALOG:
		keypro_init(hwnd);
//		InitReturnPaperbox(hwnd);
		CheckRadioButton(hwnd, IDC_BKSBS, IDC_BKSLEFT,
				IDC_BKSBS + cfg.bksp_is_delete);

		CheckRadioButton(hwnd, IDC_DELETERETURN, IDC_DELETEDEL,
				cfg.deleteisreturn == 1 ? IDC_DELETERETURN : IDC_DELETEDEL);
		if (cfg.allowshortcuts == 1) {
			EnableShortCuts(hwnd, TRUE);
			CheckDlgButton(hwnd, IDC_ALLOWSHORTCUTS, BST_CHECKED);
		} else if (cfg.allowshortcuts == 0) {
			CheckDlgButton(hwnd, IDC_ALLOWSHORTCUTS, BST_UNCHECKED);
			EnableShortCuts(hwnd, FALSE);
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
//		case IDC_RETURNPAPER: // 组合框控件的通知消息
//			if (HIWORD(wParam) == CBN_SELCHANGE) //选中IP地址自动调充端口号
//			{
//				cfg.returnpaper = (SendMessage(
//						GetDlgItem(hwnd, IDC_RETURNPAPER), CB_GETCURSEL, 0, 0)
//						+ 1);
//			}
//			break;
		case IDC_BKSBS:
			cfg.bksp_is_delete = 0;
			break;
		case IDC_BKSDEL:
			cfg.bksp_is_delete = 1;
			break;
		case IDC_BKSLEFT:
			cfg.bksp_is_delete = 2;
			break;
		case IDC_DELETEDEL:
			cfg.deleteisreturn = 0;
			break;
		case IDC_DELETERETURN:
			cfg.deleteisreturn = 1;
			break;
		case IDC_ALLOWSHORTCUTS:
			if (IsDlgButtonChecked(hwnd, IDC_ALLOWSHORTCUTS) == BST_CHECKED) {
				cfg.allowshortcuts = 1;
				EnableShortCuts(hwnd, TRUE);

			} else {
				cfg.allowshortcuts = 0;
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


	SetDlgItemTextA(hwnd, IDC_SYSUSER, cfg.sysuser);
	SetDlgItemTextA(hwnd, IDC_SYSPASS, cfg.syspass);
	if (cfg.currentuser == 1) {
		EnableCurrentUser(hwnd, false);
		CheckDlgButton(hwnd, IDC_CURRENTUSER, BST_CHECKED);
	} else {
		EnableCurrentUser(hwnd, true);
		CheckDlgButton(hwnd, IDC_CURRENTUSER, BST_UNCHECKED);
	}
	CheckDlgButton(hwnd, IDC_DEBUG, cfg.debug);
	SetDlgItemTextA(hwnd, IDC_UPDATEIP, cfg.updateip);

	SendMessageA(GetDlgItem(hwnd, IDC_ADMINPASSWORD), EM_LIMITTEXT, 6, 0);
	SetDlgItemTextA(hwnd, IDC_ADMINPASSWORD, cfg.adminpassword);
	SetDlgItemInt(hwnd, IDC_UPDATEPORT, cfg.updateport, false);
	CheckRadioButton(hwnd, IDC_CURSONTYPE0, IDC_CURSONTYPE2,
			IDC_CURSONTYPE0 + cfg.cursor_type);

	SetDlgItemInt(hwnd, IDC_FONTHEIGHT, cfg.font.height, false);
	SetDlgItemInt(hwnd, IDC_FONTWIDTH, cfg.fontwidth, false);

	CheckDlgButton(hwnd, IDC_NONUMLOCK, cfg.no_applic_k);
	if (cfg.beep == 1) {
		CheckRadioButton(hwnd, IDC_DEFAULT, IDC_WAVFILE, IDC_DEFAULT);
		EnableWindow(GetDlgItem(hwnd, IDC_FILEPATH), FALSE);
	} else if (cfg.beep == 3) {
		CheckRadioButton(hwnd, IDC_DEFAULT, IDC_WAVFILE, IDC_WAVFILE);
		SetDlgItemTextA(hwnd, IDC_FILEPATH, cfg.bell_wavefile.path);
	}
	CheckDlgButton(hwnd, IDC_TEXTBLINK, cfg.blinktext);
	CheckDlgButton(hwnd, IDC_LOGTYPE,
			cfg.logtype == 2 ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, IDC_WARNONCLOSE, cfg.close_on_exit);
	CheckDlgButton(hwnd, IDC_AUTOCOPY, cfg.autocopy);
	CheckDlgButton(hwnd, IDC_CURSONBLINK, cfg.blink_cur);
	CheckDlgButton(hwnd, IDC_CHECKDISPLAY, cfg.cursor_on);
	CheckDlgButton(hwnd, IDC_ALLOWREPEAT, cfg.allowrepeat);
//	CheckDlgButton(hwnd, IDC_ENABLESCREENNUM, cfg.enablescreennum);
	SendMessage(GetDlgItem(hwnd, IDC_LINECODEPAGE), CB_ADDSTRING, 0,
			(LPARAM) TEXT("UTF-8"));
	SendMessage(GetDlgItem(hwnd, IDC_LINECODEPAGE), CB_ADDSTRING, 0,
			(LPARAM) TEXT("GBK"));

	SendMessage(GetDlgItem(hwnd, IDC_LINECODEPAGE), WM_SETTEXT, 0,
			(LPARAM) (cfg.line_codepage));

}

static int CALLBACK TabSheet2Proc(HWND hwnd, UINT msg, WPARAM wParam,
		LPARAM lParam) {
	COLORREF result;
	COLORREF g_rgbCustom[16]; //16个颜色存储，指定对话框的16个颜色选择
	CHOOSECOLOR cc;
	switch (msg) {
	case WM_INITDIALOG: {
		ConfigDialog::InitTabSheet2(hwnd);
		SetDlgItemTextA(hwnd, IDC_FILEPATH, cfg.bell_wavefile.path);
		return true;
	}

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DEBUG:
			if (IsDlgButtonChecked(hwnd, IDC_DEBUG) == BST_CHECKED) {
				cfg.debug = 1;
			} else {
				cfg.debug = 0;
			}
			break;
		case IDC_CURRENTUSER:
			if (IsDlgButtonChecked(hwnd, IDC_CURRENTUSER) == BST_CHECKED) {
				cfg.currentuser = 1;
				EnableCurrentUser(hwnd, false);
			} else {
				cfg.currentuser = 0;
				EnableCurrentUser(hwnd, true);
			}
			break;
		case IDC_CURSONTYPE0:
			cfg.cursor_type = 0;
			break;
		case IDC_CURSONTYPE1:
			cfg.cursor_type = 1;
			break;
		case IDC_CURSONTYPE2:
			cfg.cursor_type = 2;
			break;
//		case IDC_ENABLESCREENNUM:
//			if (IsDlgButtonChecked(hwnd, IDC_ENABLESCREENNUM) == BST_CHECKED)
//				cfg.enablescreennum = 1;
//			else
//				cfg.enablescreennum = 0;
//			break;
		case IDC_CHECKDISPLAY:
			if (IsDlgButtonChecked(hwnd, IDC_CHECKDISPLAY) == BST_CHECKED)
				cfg.cursor_on = 1;
			else
				cfg.cursor_on = 0;
			break;
		case IDC_CURSONBLINK:
			if (IsDlgButtonChecked(hwnd, IDC_CURSONBLINK) == BST_CHECKED)
				cfg.blink_cur = 1;
			else
				cfg.blink_cur = 0;
			break;
		case IDC_DEFAULT:
			EnableWindow(GetDlgItem(hwnd, IDC_FILEPATH), FALSE);
			cfg.beep = 1;
			break;
		case IDC_WAVFILE:
			EnableWindow(GetDlgItem(hwnd, IDC_FILEPATH), TRUE);
			cfg.beep = 3;
			break;
		case IDC_TEXTBLINK:
			if (IsDlgButtonChecked(hwnd, IDC_TEXTBLINK) == BST_CHECKED)
				cfg.blinktext = 1;
			else
				cfg.blinktext = 0;
			break;
		case IDC_LOGTYPE:
			if (IsDlgButtonChecked(hwnd, IDC_LOGTYPE) == BST_CHECKED)
				cfg.logtype = 2;
			else
				cfg.logtype = 0;
			break;
		case IDC_WARNONCLOSE:
			if (IsDlgButtonChecked(hwnd, IDC_WARNONCLOSE) == BST_CHECKED)
				cfg.close_on_exit = 1;
			else
				cfg.close_on_exit = 0;
			break;
		case IDC_AUTOCOPY:
			if (IsDlgButtonChecked(hwnd, IDC_AUTOCOPY) == BST_CHECKED)
				cfg.autocopy = 1;
			else
				cfg.autocopy = 0;
			break;
		case IDC_NONUMLOCK:
			if (IsDlgButtonChecked(hwnd, IDC_NONUMLOCK) == BST_CHECKED)
				cfg.no_applic_k = 1;
			else
				cfg.no_applic_k = 0;
			break;
		case IDC_ALLOWREPEAT:
			if (IsDlgButtonChecked(hwnd, IDC_ALLOWREPEAT) == BST_CHECKED) {
				cfg.allowrepeat = 1;
//				MessageBoxA(hwnd, REPEAT, APPLICATION_NAME, MB_OK);
			} else
				cfg.allowrepeat = 0;
			break;
		case IDC_BUTTONBACKGROUND:
		case IDC_BUTTONFONT:
		case IDC_BUTTONEDGEC:
		case IDC_BUTTONCURSON:
			for (iCircle = 0; iCircle < 16; iCircle++) {
				g_rgbCustom[iCircle] = RGB(255,255,255); //全部初始化为白色
			}
			cc.lStructSize = sizeof(CHOOSECOLOR);
			cc.rgbResult = RGB(0,0,0); //默认选中颜色
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
//				colours_tocfg();
			}
			break;
		case IDC_BROWSE:
			if (Util::SelectFolder(hwnd, cfg.bell_wavefile.path, SELECTWAVFILE,
					BIF_BROWSEINCLUDEFILES))
				SetDlgItemTextA(hwnd, IDC_FILEPATH, cfg.bell_wavefile.path);
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
	SetDlgItemTextA(hwnd, IDC_EDITESC, cfg.keycodeesc);
	SetDlgItemTextA(hwnd, IDC_EDITINSERT, cfg.keycodeinsert);
	SetDlgItemTextA(hwnd, IDC_EDITDELETE, cfg.keycodedelete);
	SetDlgItemTextA(hwnd, IDC_EDITHOME, cfg.keycodehome);
	SetDlgItemTextA(hwnd, IDC_EDITNUMLOCK, cfg.keycodenumlock);
	SetDlgItemTextA(hwnd, IDC_EDITBACKSPACE, cfg.keycodespace);
	SetDlgItemTextA(hwnd, IDC_EDITPRINTSCREEN, cfg.keycodeprintscreen);
	SetDlgItemTextA(hwnd, IDC_EDITSCROLL, cfg.keycodescroll);
	SetDlgItemTextA(hwnd, IDC_EDITEND, cfg.keycodeend);
	SetDlgItemTextA(hwnd, IDC_EDITPAGEUP, cfg.keycodepageup);
	SetDlgItemTextA(hwnd, IDC_EDITPAGEDOWN, cfg.keycodepagedown);
	SetDlgItemTextA(hwnd, IDC_EDITADD, cfg.keycodeadd);
	SetDlgItemTextA(hwnd, IDC_EDITSUB, cfg.keycodesub);
	SetDlgItemTextA(hwnd, IDC_EDITDIV, cfg.keycodediv);
	SetDlgItemTextA(hwnd, IDC_EDITMULT, cfg.keycodemult);
	SetDlgItemTextA(hwnd, IDC_EDITPAUSE, cfg.keycodepause);
	SetDlgItemTextA(hwnd, IDC_EDITUP, cfg.keycodeup);
	SetDlgItemTextA(hwnd, IDC_EDITDOWN, cfg.keycodedown);
	SetDlgItemTextA(hwnd, IDC_EDITLEFT, cfg.keycodeleft);
	SetDlgItemTextA(hwnd, IDC_EDITRIGHT, cfg.keycoderight);
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
		if (cfg.allowfunction == 1) {
			CheckDlgButton(hwnd, IDC_ALLOWFUNCTION, BST_CHECKED);
			EnableFunction(hwnd, TRUE);
		} else if (cfg.allowfunction == 0) {
			CheckDlgButton(hwnd, IDC_ALLOWFUNCTION, BST_UNCHECKED);
			EnableFunction(hwnd, FALSE);
		}
		return 1;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ALLOWFUNCTION:
			if (IsDlgButtonChecked(hwnd, IDC_ALLOWFUNCTION) == BST_CHECKED) {
				cfg.allowfunction = 1;
				EnableFunction(hwnd, TRUE);
			} else {
				cfg.allowfunction = 0;
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
				&cfg) == 0)
			return;

		cfg.com01y = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM1Y, 3);
		cfg.com02z = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM2Z, 3);
		cfg.com03x = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM3X, 3);
		cfg.com04u = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM4U, 3);
		cfg.com05w = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM5W, 3);
		cfg.com06v = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_COM6V, 3);
		cfg.printcom = ConfigDialog::GetComSet(g_hTabSheet[0], IDC_PRINTCOM, 3);
		if (cfg.autologin == 1) {
			GetDlgItemText(g_hTabSheet[0], IDC_LOGINPROMPT, cfg.loginprompt,
					128);
			GetDlgItemText(g_hTabSheet[0], IDC_PASSWORDPROMPT,
					cfg.passwordprompt, 128);
			GetDlgItemText(g_hTabSheet[0], IDC_USERNAME, cfg.autousername,
					NAME_LEN);
			GetDlgItemText(g_hTabSheet[0], IDC_PASSWORD, cfg.autopassword,
					NAME_LEN);
		}
		cfg.ping_interval = GetDlgItemInt(g_hTabSheet[0], IDC_PINGINTERVAL,
				NULL, false);
		if (cfg.printmode == 0) {
			GetDlgItemTextA(g_hTabSheet[0], IDC_PRINTERNAME, cfg.printer,
					NAME_LEN);
		} else if (cfg.printmode == 3) {
			GetDlgItemTextA(g_hTabSheet[0], IDC_PRINTERNAME, cfg.fprintname,
					NAME_LEN);
			GetDlgItemTextA(g_hTabSheet[0], IDC_EPSHARE, cfg.fpshare, NAME_LEN);
		}
		GetDlgItemTextA(g_hTabSheet[0], IDC_SCREENNUM, cfg.screennum, NAME_LEN);

		if ((cfg.printmode == 0) && (!*cfg.printer)) {
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
		cfg.font.height = GetDlgItemInt(g_hTabSheet[2], IDC_FONTHEIGHT, NULL,
				false);
		cfg.fontwidth = GetDlgItemInt(g_hTabSheet[2], IDC_FONTWIDTH, NULL,
				false);
		GetDlgItemText(g_hTabSheet[2], IDC_LINECODEPAGE, cfg.line_codepage,
				128);
		GetDlgItemText(g_hTabSheet[2], IDC_UPDATEIP, cfg.updateip, 32);
		cfg.updateport = GetDlgItemInt(g_hTabSheet[2], IDC_UPDATEPORT, NULL,
				FALSE);
		GetDlgItemText(g_hTabSheet[2], IDC_ADMINPASSWORD, cfg.adminpassword,
				128);
		GetDlgItemText(g_hTabSheet[2], IDC_SYSUSER, cfg.sysuser, 128);
		GetDlgItemText(g_hTabSheet[2], IDC_SYSPASS, cfg.syspass, 128);

		Util::KillProcess("update.exe");
		Sleep(1000);
		char exepath[MAX_PATH];
		sprintf(exepath, "%s\\update.exe", currentpath);
		if (Util::FileExists(exepath))
			Util::ShellExecuteNoWait(exepath);
		break;
	case 3:
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITESC, cfg.keycodeesc, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITINSERT, cfg.keycodeinsert,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITDELETE, cfg.keycodedelete,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITHOME, cfg.keycodehome, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITBACKSPACE, cfg.keycodespace,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITNUMLOCK, cfg.keycodenumlock,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPRINTSCREEN,
				cfg.keycodeprintscreen, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITSCROLL, cfg.keycodescroll,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITEND, cfg.keycodeend, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPAGEUP, cfg.keycodepageup,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPAGEDOWN, cfg.keycodepagedown,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITADD, cfg.keycodeadd, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITSUB, cfg.keycodesub, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITDIV, cfg.keycodediv, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITMULT, cfg.keycodemult, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITPAUSE, cfg.keycodepause,
				KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITUP, cfg.keycodeup, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITDOWN, cfg.keycodedown, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITLEFT, cfg.keycodeleft, KEY_LEN);
		GetDlgItemTextA(g_hTabSheet[3], IDC_EDITRIGHT, cfg.keycoderight,
				KEY_LEN);
		Update_KeyPairs();
		break;
	default:
		break;
	}

	Setting::save_config();
	Setting::save_settings(sectionName, &cfg);
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

	g_hTabSheet[0] =
			CreateDialogA(hinst,(LPCTSTR)IDD_DIALOG_SHEET1,hwnd,(DLGPROC)TabSheet0Proc);
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

//			SHELLEXECUTEINFO si = { sizeof(SHELLEXECUTEINFO) };
//			string path;
//			path = getenv("PATH");
//
//			vector<string> temp = Util::Split(path, ';');
//			for (int i = 0; i < temp.size(); i++) {
//				string ewfpath;
//				ewfpath = temp.at(i) + "\\ewfmgr.exe";
//				if (Util::FileExists(ewfpath)) {
//					si.fMask = SEE_MASK_NOCLOSEPROCESS;
//					si.lpVerb = "open";
//					si.lpDirectory = ".";
//					si.lpFile = "ewfmgr";
//					si.lpParameters = "c: -commit";
//				}
//			}

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
			Setting::save_settings(sectionName, &cfg);
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
		memset(password, 0x00, 50);
		SendMessageA(GetDlgItem(hwnd, IDC_PASSWORD), EM_LIMITTEXT, 6, 0);
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemTextA(hwnd, IDC_PASSWORD, password, 7);
			if (memcmp(password, cfg.adminpassword, 6) == 0) {
				cfg.admin = 1;
			} else {
				cfg.admin = 0;
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
	memcpy(iptemp, cfg.host, 512);
	porttemp = cfg.port;
	switch (msg) {
	case WM_INITDIALOG: {

		int i = 1;
		strcpy(szPath, cfg.linkpath);
		if (strcmp(szPath, "") == 0) {
			Util::GetDesktopPath(szPath);
			CheckRadioButton(hwnd, IDC_SELECT, IDC_DESKTOP, IDC_DESKTOP);
			EnableWindow(GetDlgItem(hwnd, IDC_SHORTCUTSPATH), FALSE);
		} else {
			CheckRadioButton(hwnd, IDC_SELECT, IDC_DESKTOP, IDC_SELECT);
			EnableWindow(GetDlgItem(hwnd, IDC_SHORTCUTSPATH), TRUE);
		}
		SetDlgItemTextA(hwnd, IDC_SHORTCUTSPATH, szPath);

//		memset(section, 0, 24);
//		do {
//			sprintf(section, "section%03d", i++);
//		} while (open_settings_r(section) != 0);
//		SetDlgItemText(hwnd, IDC_SECTIONNAME, section);
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

			if (InputIPSuccess(hwnd, IDC_TERMIP, IDC_TERMPORT, &cfg) == 0)
				return 0;
			CoInitialize(NULL);
			memset(screennumtemp, 0x00, 128);
			memcpy(screennumtemp, cfg.screennum, 128);
			char exepath[128];
			sprintf(exepath, "%s\\shell.exe", currentpath);
			if (CreateShortCut(exepath, section, szPath, description,
					SW_SHOWNORMAL, currentpath, icopath, 0) == 0) {
				GetDlgItemTextA(hwnd, IDC_NEWSCREENNUM, cfg.screennum, 128);
				MessageBoxA(hwnd, NEWTERMSUCCESS, TITLE_POINT, MB_OK);
				Setting::save_settings(section, &cfg);
				Setting::save_config();
			}
			memcpy(cfg.host, iptemp, 512);
			cfg.port = porttemp;
			memset(cfg.screennum, 0x00, 128);
			memcpy(cfg.screennum, screennumtemp, 128);
			Setting::save_settings(sectionName, &cfg);
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

} /* namespace terminal */
//static int CALLBACK TipProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	switch (msg) {
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	case WM_INITDIALOG:
//		return 1;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDC_SET:
//			terminal::ConfigDialog::ShowSet(hwnd);
////			terminal::ConfigDialog::ShowConnection(hwnd);
//			break;
//		case IDOK:
//			IsNewConnection = 1;
//			EndDialog(hwnd, TRUE);
//			break;
//		case IDCANCEL:
//			IsNewConnection = 0;
//			EndDialog(hwnd, TRUE);
//			break;
//		}
//		return 0;
//	}
//	return 0;
//}
//

//
//void ShowTip(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_TIP), hwnd, terminal::TipProc);
//}

                             
