/*
 * windlg.c - dialogs for PuTTY(tel), including the configuration dialog.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#include "../putty.h"
#include "win_res.h"
#include "../storage.h"
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include "../resources/resource.h"
#include "message.h"

#ifdef MSVC4
#define TVINSERTSTRUCT  TV_INSERTSTRUCT
#define TVITEM          TV_ITEM
#define ICON_BIG        1
#endif

///*
// * These are the various bits of data required to handle the
// * portable-dialog stuff in the config box. Having them at file
// * scope in here isn't too bad a place to put them; if we were ever
// * to need more than one config box per process we could always
// * shift them to a per-config-box structure stored in GWL_USERDATA.
// */
//static struct controlbox *ctrlbox;
///*
// * ctrls_base holds the OK and Cancel buttons: the controls which
// * are present in all dialog panels. ctrls_panel holds the ones
// * which change from panel to panel.
// */
//static struct winctrls ctrls_base, ctrls_panel;
//static struct dlgparam dp;
//
static char **events = NULL;
static int nevents = 0, negsize = 0;
//
//extern Config cfg; /* defined in window.c */
//
//#define PRINTER_DISABLED_STRING "None (printing disabled)"
//
//extern int comHandle[4];
//extern int IsNewConnection;
//extern char sectionName[256];
//extern Config cfg;
//
//int iItemIndex; //下拉列表框记录当前索引
//char port[8];
//int iCircle;
//
//extern unsigned char KeyCodeFXX[12][8];
//extern int KeyCodeLenFXX[12];
//
//void force_normal(HWND hwnd) {
//	static int recurse = 0;
//
//	WINDOWPLACEMENT wp;
//
//	if (recurse)
//		return;
//	recurse = 1;
//
//	wp.length = sizeof(wp);
//	if (GetWindowPlacement(hwnd, &wp) && wp.showCmd == SW_SHOWMAXIMIZED) {
//		wp.showCmd = SW_SHOWNORMAL;
//		SetWindowPlacement(hwnd, &wp);
//	}
//	recurse = 0;
//}
//
//static int CALLBACK RegProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
////	char InputKey[128];
////
////	switch (msg) {
////	case WM_CLOSE:
////		exit(1);
////		EndDialog(hwnd, TRUE);
////		return 0;
////	case WM_INITDIALOG:
////		SetDlgItemText(hwnd, IDC_STATICMACHCODE, jiqiNumber);
////		return 1;
////	case WM_COMMAND:
////		switch (LOWORD(wParam)) {
////		case IDC_MACHCODE:
////			if (OpenClipboard(hwnd)) {
////				HGLOBAL hmem = GlobalAlloc(GMEM_DDESHARE, 256);
////				char *pmem = GlobalLock(hmem);
////				EmptyClipboard();
////				memcpy(pmem, jiqiNumber, 256);
////				GlobalUnlock(hmem);
////				SetClipboardData(CF_TEXT, hmem);
////				CloseClipboard();
////				//GlobalFree(hmem);
////				MessageBox(NULL, "机器码已写入剪贴板中", COMPANY_NAME,
////						MB_OK | MB_ICONINFORMATION);
////			}
////			break;
////		case IDC_REG:
////			memset(InputKey, 0x00, 128);
////			GetDlgItemText(hwnd, IDC_REGCODE, InputKey, 128);
////			if (memcmp(outData, InputKey,
////					strlen(outData) > strlen(InputKey) ?
////							strlen(outData) : strlen(InputKey)) != 0) {
////				MessageBox(NULL, "注册码不正确，请重新输入", COMPANY_NAME,
////						MB_OK | MB_ICONWARNING);
////				memset(InputKey, 0x00, 128);
////				SetDlgItemText(hwnd, IDC_REGCODE, InputKey);
////				return 0;
////			}
////			memcpy(cfg.registcod,outData,1024);
////			memcpy(cfg.registkey,jiqiNumber,1024);
////			EndDialog(hwnd, TRUE);
////			break;
////		case IDCANCEL:
////			exit(1);
////			EndDialog(hwnd, TRUE);
////			break;
////		}
////		return 0;
////	}
//	return 0;
//}
//
//void ShowReg(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_REG), hwnd, RegProc);
//}
//
//static int CALLBACK InterfaceProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//	long lReg;
//	HKEY hKey;
//	DWORD MaxValueLength;
//	DWORD dwValueNumber;
//	DWORD cchValueName, dwValueSize = 6;
//	char pValueName[32], pCOMNumber[32];
//	char ComValue[8];
//
//	switch (msg) {
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	case WM_INITDIALOG:
//		lReg = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
//				"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_QUERY_VALUE, &hKey); //打开注册表键值
//		if (lReg != ERROR_SUCCESS) {
//			return 0;
//		}
//		lReg = RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
//				&dwValueNumber, //返回和hKey关联的值
//				&MaxValueLength, NULL, NULL, NULL);
//		if (lReg != ERROR_SUCCESS) //没有成功
//		{
//			return 0;
//		}
//
//		for (iCircle = 0; iCircle < dwValueNumber; iCircle++) {
//			cchValueName = MaxValueLength + 1;
//			dwValueSize = 6;
//			lReg = RegEnumValueA(hKey, iCircle, pValueName, &cchValueName, NULL,
//					NULL, NULL, NULL);
//
//			if ((lReg != ERROR_SUCCESS) && (lReg != ERROR_NO_MORE_ITEMS)) {
//				continue;
//			}
//
//			lReg = RegQueryValueExA(hKey, pValueName, NULL, NULL,
//					(LPBYTE) pCOMNumber, &dwValueSize);
//
//			memset(ComValue, 0x00, 8);
//			memcpy(ComValue, pCOMNumber + 3, strlen(pCOMNumber) - 3);
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_PWDKEY, CB_ADDSTRING, 0,
//					pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_PWDKEY), CB_SETITEMDATA,
//					iItemIndex, atoi(ComValue));
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_READCARD, CB_ADDSTRING, 0,
//					pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_READCARD), CB_SETITEMDATA,
//					iItemIndex, atoi(ComValue));
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_TELLERCARD, CB_ADDSTRING,
//					0, pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_TELLERCARD), CB_SETITEMDATA,
//					iItemIndex, atoi(ComValue));
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_CERTCARD, CB_ADDSTRING, 0,
//					pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_CERTCARD), CB_SETITEMDATA,
//					iItemIndex, atoi(ComValue));
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_ZHIWEN, CB_ADDSTRING, 0,
//					pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_ZHIWEN), CB_SETITEMDATA,
//					iItemIndex, atoi(ComValue));
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_ZHIFU, CB_ADDSTRING, 0,
//					pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_ZHIFU), CB_SETITEMDATA, iItemIndex,
//					atoi(ComValue));
//			iItemIndex = SendDlgItemMessage(hwnd, IDC_OTHER, CB_ADDSTRING, 0,
//					pCOMNumber);
//			SendMessage(GetDlgItem(hwnd, IDC_OTHER), CB_SETITEMDATA, iItemIndex,
//					atoi(ComValue));
//
//		}
//
////		memset(ComValue, 0x00, 8);
////		memcpy(ComValue, cfg.com01, 8);
////		SetDlgItemText(hwnd, IDC_PWDKEY, ComValue);
////		memset(ComValue, 0x00, 8);
////		memcpy(ComValue, cfg.com02, 8);
////		SetDlgItemText(hwnd, IDC_READCARD, ComValue);
////
////		{
////			int i;
////			for (i = 0; i < 5; i++) {
////				memset(ComValue, 0x00, 8);
////				memcpy(ComValue, cfg.com03, 8);
////				SetDlgItemText(hwnd, IDC_TELLERCARD + i, ComValue);
////
////			}
////		}
//
//		return 1;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDOK:
//			memset(ComValue, 0x00, 8);
//			GetDlgItemText(hwnd, IDC_PWDKEY, ComValue, 8);
////			memcpy(cfg.com01, ComValue, 8);
////			com[0] = cfg.com01;
////			memset(ComValue, 0x00, 8);
////			GetDlgItemText(hwnd, IDC_READCARD, ComValue, 8);
////			memcpy(cfg.com02, ComValue, 8);
////			com[1] = cfg.com02;
////			memset(ComValue, 0x00, 8);
////			GetDlgItemText(hwnd, IDC_TELLERCARD, ComValue, 8);
////			memcpy(cfg.com03, ComValue, 8);
////			com[2] = cfg.com03;
//			save_settings(sectionName, &cfg);
//			MessageBox(NULL, SAVE_SET_PORT, COMPANY_NAME,
//					MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1);
//		case IDCANCEL:
//			EndDialog(hwnd, TRUE);
//			break;
//
//		}
//	}
//	return 0;
//}
//
//void ShowInterface(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_INTERFACE), hwnd, InterfaceProc);
//}
//
//int backR;
//int backG;
//int backB;
//int fontR;
//int fontG;
//int fontB;
//int boderR;
//int boderG;
//int boderB;
//
//void colours_tocfg() {
//	cfg.colours[2][0] = backR;
//	cfg.colours[2][1] = backG;
//	cfg.colours[2][2] = backB;
//	cfg.colours[0][0] = fontR;
//	cfg.colours[0][1] = fontG;
//	cfg.colours[0][2] = fontB;
//	cfg.colours[1][0] = boderR;
//	cfg.colours[1][1] = boderG;
//	cfg.colours[1][2] = boderB;
//
//}
//
//void cfg_tocolours() {
//	backR = cfg.colours[2][0];
//	backG = cfg.colours[2][1];
//	backB = cfg.colours[2][2];
//	fontR = cfg.colours[0][0];
//	fontG = cfg.colours[0][1];
//	fontB = cfg.colours[0][2];
//	boderR = cfg.colours[1][0];
//	boderG = cfg.colours[1][1];
//	boderB = cfg.colours[1][2];
//}
//
//void colours_resumvalue() {
//
//	backR = 0;
//	backG = 128;
//	backB = 64;
//	fontR = 255;
//	fontG = 255;
//	fontB = 255;
//	boderR = 255;
//	boderG = 255;
//	boderB = 255;
//}
//
//static int CALLBACK ShowSettingProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//	COLORREF result;
//	COLORREF g_rgbCustom[16]; //16个颜色存储，指定对话框的16个颜色选择
//	CHOOSECOLOR cc;
//	char color[4];
//	HDC hdc;
//	switch (msg) {
//	//case WM_CTLCOLORBTN:
//	//	return (LONG)CreateSolidBrush(RGB(111, 111, 111));
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	case WM_INITDIALOG:
//		cfg_tocolours();
//		return 0;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDC_BUTTONBACKGROUND:
//		case IDC_BUTTONFONT:
//		case IDC_BUTTONEDGE:
//			for (iCircle = 0; iCircle < 16; iCircle++) {
//				g_rgbCustom[iCircle] = RGB(255,255,255); //全部初始化为白色
//			}
//			cc.lStructSize = sizeof(CHOOSECOLOR);
//			cc.rgbResult = RGB(0,0,0); //默认选中颜色
//			cc.Flags = CC_RGBINIT | CC_FULLOPEN | CC_ANYCOLOR | CC_SOLIDCOLOR;
//			cc.hwndOwner = hwnd;
//			cc.lpCustColors = g_rgbCustom;
//			if (ChooseColor(&cc)) {
//				result = cc.rgbResult;
//				if (LOWORD(wParam) == IDC_BUTTONBACKGROUND) {
//					backR = GetRValue(result);
//					backG = GetGValue(result);
//					backB = GetBValue(result);
//				} else if (LOWORD(wParam) == IDC_BUTTONFONT) {
//					fontR = GetRValue(result);
//					fontG = GetGValue(result);
//					fontB = GetBValue(result);
//				} else if (LOWORD(wParam) == IDC_BUTTONEDGE) {
//					boderR = GetRValue(result);
//					boderG = GetGValue(result);
//					boderB = GetBValue(result);
//				}
//			}
//			break;
//		case IDC_RESUMVALUE:
//			colours_resumvalue();
//			colours_tocfg();
//			save_settings(sectionName, &cfg);
//
//			MessageBox(NULL, RESUME_SET_COLOUR, COMPANY_NAME,
//					MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1);
//			break;
//		case IDOK:
//			colours_tocfg();
//			save_settings(sectionName, &cfg);
//			MessageBox(NULL, SAVE_SET_COLOUR, COMPANY_NAME,
//					MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1);
//		case IDCANCEL:
//			cfg_tocolours();
//
//			EndDialog(hwnd, TRUE);
//			break;
//		}
//	}
//	return 0;
//}
//
//void ShowShowSetting(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_SHOWSETTING), hwnd, ShowSettingProc);
//
//}
//
//static int CALLBACK AddIpProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	char textIPValue[32]; //存放窗体中IP值
//	char textPORTValue[32]; //存放窗体中PORT值
//	char severHost[32];
//	char jieName[32];
//	int rv = 0;
//
//	switch (msg) {
//	case WM_INITDIALOG:
//		return 1;
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDOK:
//			//取得窗体输入的IP地址
//			memset(textIPValue, 0x00, 32);
//			GetDlgItemText(hwnd, IDC_IPADDRESSVALUE, textIPValue, 32);
//			//取得窗体输入的端口号
//			memset(textPORTValue, 0x00, 32);
//			GetDlgItemText(hwnd, IDC_EDITPORT, textPORTValue, 32);
//			memset(severHost, 0x00, 32);
//			severHost[0] = '0';
//			severHost[1] = '.';
//			severHost[2] = '0';
//			severHost[3] = '.';
//			severHost[4] = '0';
//			severHost[5] = '.';
//			severHost[6] = '0';
//			if (memcmp(severHost, textIPValue, sizeof(textIPValue)) == 0) {
//				MessageBox(NULL, WARNING_HOST_EMPTY, COMPANY_NAME,
//						MB_OK | MB_ICONWARNING | MB_DEFBUTTON1);
//				return 0;
//			}
//			if (strlen(textPORTValue) == 0 || strlen(textPORTValue) > 5) {
//				MessageBox(NULL, WARNING_PORT_WRONG, COMPANY_NAME,
//						MB_OK | MB_ICONWARNING | MB_DEFBUTTON1);
//				return 0;
//			}
//			memset(severHost, 0x00, 32);
//			memcpy(severHost, cfg.host, strlen(cfg.host));
//			if (memcmp(severHost, textIPValue, sizeof(textIPValue)) == 0) {
//				memset(severHost, 0x00, 32);
//				sprintf(severHost, "%d", cfg.port);
//				if (memcmp(severHost, textPORTValue, sizeof(textPORTValue))
//						== 0) {
//					MessageBox(NULL, WARNING_HOST_EXIST, COMPANY_NAME,
//							MB_OK | MB_ICONWARNING | MB_DEFBUTTON1);
//					return 0;
//				}
//			}
//			memcpy(cfg.host, textIPValue, strlen(textIPValue));
//			cfg.port = atoi(textPORTValue);
//			iItemIndex = SendDlgItemMessage(GetParent(hwnd), IDC_IPADDRESS,
//					CB_ADDSTRING, 0, textIPValue);
//			SendMessage(GetDlgItem(GetParent(hwnd), IDC_IPADDRESS),
//					CB_SETITEMDATA, iItemIndex, atoi(textPORTValue));
//			save_settings(sectionName, &cfg);
//			MessageBox(NULL, SUCCESS_ADD_HOSTPORT, COMPANY_NAME,
//					MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1);
//			SendDlgItemMessage(hwnd, IDC_IPADDRESS, CB_ADDSTRING, 0, severHost);
//			SetDlgItemText(hwnd, IDC_EDITPORT, severHost);
//		case IDCANCEL:
//			EndDialog(hwnd, TRUE);
//			break;
//		}
//		return 0;
//	}
//	return 0;
//}
//
//void ShowAddIp(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_ADDIPADDRESS), hwnd, AddIpProc);
//}
//
//static int CALLBACK ConnectionProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//	char textIPValue[32]; //存放窗体中IP值
//	char textPORTValue[32]; //存放窗体中PORT值
//	char severHost[32];
//	char jieName[32];
//	char tempJieName[32];
//	int rv = 0;
//
//	switch (msg) {
//	case WM_INITDIALOG:
//		//memset(severHost,0x00,32);
//		//rv=readIniStr("SOCKET","serverIp",severHost,TEMINAL_CONFIG_FILE);
//		//iItemIndex=SendDlgItemMessage(hwnd,IDC_IPADDRESS,CB_ADDSTRING,0,severHost);
//		//SendMessage(GetDlgItem(hwnd,IDC_IPADDRESS),CB_SETITEMDATA,iItemIndex,readIniInt("SOCKET","port",TEMINAL_CONFIG_FILE));
//		memset(severHost, 0x00, 32);
//		memcpy(severHost, cfg.host, strlen(cfg.host));
//		iItemIndex = SendDlgItemMessage(hwnd, IDC_IPADDRESS, CB_ADDSTRING, 0,
//				severHost);
//		SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS), CB_SETITEMDATA, iItemIndex,
//				cfg.port);
//		memset(severHost, 0x00, 32);
//		sprintf(severHost, "%d", cfg.port);
//		SetDlgItemText(hwnd, IDC_EDITPORT, severHost);
//		SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS), CB_SETCURSEL, 0, 0); //设置下拉列表框默认选中第一个
//		return 1;
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDC_IPADDRESS:
//			if (HIWORD(wParam) == CBN_SELCHANGE) //选中IP地址自动调充端口号
//			{
//				iItemIndex = SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS),
//						CB_GETCURSEL, 0, 0);
//				iItemIndex = SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS),
//						CB_GETITEMDATA, iItemIndex, 0);
//				memset(port, 0x00, 8);
//				sprintf(port, "%d", iItemIndex);
//				SetDlgItemText(hwnd, IDC_EDITPORT, port);
//			}
//			break;
//		case IDCANCEL:
//			EndDialog(hwnd, TRUE);
//			break;
//		case IDC_ADD:
//			ShowAddIp(hwnd);
//			break;
//		case IDC_DEL:
//			iItemIndex = SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS),
//					CB_GETCURSEL, 0, 0);
//			SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS), CB_DELETESTRING,
//					iItemIndex, 0);
//			memset(cfg.host, 0x00, 32);
//			cfg.port = 0;
//			SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS), CB_SETCURSEL, -1, 0);
//			SetDlgItemText(hwnd, IDC_EDITPORT, "");
//			MessageBox(NULL, SUCCESS_DEL_HOST, COMPANY_NAME,
//					MB_OK | MB_ICONINFORMATION | MB_DEFBUTTON1);
//			save_settings(sectionName, &cfg);
//			break;
//		case IDC_DEFAULT:
//			//取得窗体输入的IP地址
//			memset(textIPValue, 0x00, 32);
//			GetDlgItemText(hwnd, IDC_IPADDRESS, textIPValue, 32);
//			//取得窗体输入的端口号
//			memset(textPORTValue, 0x00, 32);
//			GetDlgItemText(hwnd, IDC_EDITPORT, textPORTValue, 32);
//			memset(severHost, 0x00, 32);
//			memcpy(severHost, cfg.host, strlen(cfg.host));
//			if (memcmp(severHost, textIPValue, sizeof(textIPValue)) == 0) {
//				memset(severHost, 0x00, 32);
//				memcpy(severHost, cfg.port, strlen(cfg.port));
//				if (memcmp(severHost, textPORTValue, sizeof(textPORTValue))
//						== 0) {
//					save_settings(sectionName, &cfg);
//				}
//			}
//			MessageBox(NULL, POINT_SELECT_HOST, COMPANY_NAME,
//					MB_OK | MB_ICONEXCLAMATION | MB_DEFBUTTON1);
//
//			break;
//		}
//		return 0;
//	}
//	return 0;
//}
//void ShowConnection(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_CONNECTION), hwnd, ConnectionProc);
//}
//
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
//			ShowConnection(hwnd);
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
////	void ShowTip(HWND hwnd) {
////		DialogBox(hinst, MAKEINTRESOURCE(IDD_TIP), hwnd, TipProc);
////	}
//
//void keypro_resumvalue() {
//	strcpy(cfg.keycodef[1], "1B4F50");
//	strcpy(cfg.keycodef[2], "1B4F51");
//	strcpy(cfg.keycodef[3], "1B4F52");
//	strcpy(cfg.keycodef[4], "1B4F53");
//	strcpy(cfg.keycodef[5], "1B4F74");
//	strcpy(cfg.keycodef[6], "1B4F75");
//	strcpy(cfg.keycodef[7], "1B4F76");
//	strcpy(cfg.keycodef[8], "1B4F6C");
//	strcpy(cfg.keycodef[9], "1B4F77");
//	strcpy(cfg.keycodef[10], "1B4F78");
//	strcpy(cfg.keycodef[11], "1B4F79");
//	strcpy(cfg.keycodef[12], "1B4F7A");
//}
//
//void keypro_init(HWND hwnd) {
//	SetDlgItemText(hwnd, IDC_EDITF1, cfg.keycodef[1]);
//	SetDlgItemText(hwnd, IDC_EDITF2, cfg.keycodef[2]);
//	SetDlgItemText(hwnd, IDC_EDITF3, cfg.keycodef[3]);
//	SetDlgItemText(hwnd, IDC_EDITF4, cfg.keycodef[4]);
//	SetDlgItemText(hwnd, IDC_EDITF5, cfg.keycodef[5]);
//	SetDlgItemText(hwnd, IDC_EDITF6, cfg.keycodef[6]);
//	SetDlgItemText(hwnd, IDC_EDITF7, cfg.keycodef[7]);
//	SetDlgItemText(hwnd, IDC_EDITF8, cfg.keycodef[8]);
//	SetDlgItemText(hwnd, IDC_EDITF9, cfg.keycodef[9]);
//	SetDlgItemText(hwnd, IDC_EDITF10, cfg.keycodef[10]);
//	SetDlgItemText(hwnd, IDC_EDITF11, cfg.keycodef[11]);
//	SetDlgItemText(hwnd, IDC_EDITF12, cfg.keycodef[12]);
//}
//
//void dlg_to_cfg(HWND hwnd) {
//	GetDlgItemText(hwnd, IDC_EDITF1, cfg.keycodef[1], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF2, cfg.keycodef[2], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF3, cfg.keycodef[3], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF4, cfg.keycodef[4], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF5, cfg.keycodef[5], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF6, cfg.keycodef[6], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF7, cfg.keycodef[7], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF8, cfg.keycodef[8], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF9, cfg.keycodef[9], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF10, cfg.keycodef[10], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF11, cfg.keycodef[11], 1024);
//	GetDlgItemText(hwnd, IDC_EDITF12, cfg.keycodef[12], 1024);
//}
//
////void set_cfgto_uchar() {
//
////	KeyCodeLenFXX[0] = strlen(cfg.keycodef01) / 2;
////	KeyCodeLenFXX[1] = strlen(cfg.keycodef02) / 2;
////	KeyCodeLenFXX[2] = strlen(cfg.keycodef03) / 2;
////	KeyCodeLenFXX[3] = strlen(cfg.keycodef04) / 2;
////	KeyCodeLenFXX[4] = strlen(cfg.keycodef05) / 2;
////	KeyCodeLenFXX[5] = strlen(cfg.keycodef06) / 2;
////	KeyCodeLenFXX[6] = strlen(cfg.keycodef07) / 2;
////	KeyCodeLenFXX[7] = strlen(cfg.keycodef08) / 2;
////	KeyCodeLenFXX[8] = strlen(cfg.keycodef09) / 2;
////	KeyCodeLenFXX[9] = strlen(cfg.keycodef10) / 2;
////	KeyCodeLenFXX[10] = strlen(cfg.keycodef11) / 2;
////	KeyCodeLenFXX[11] = strlen(cfg.keycodef12) / 2;
////
////	tranKeyCode(cfg.keycodef01, KeyCodeFXX[0]);
////	tranKeyCode(cfg.keycodef02, KeyCodeFXX[1]);
////	tranKeyCode(cfg.keycodef03, KeyCodeFXX[2]);
////	tranKeyCode(cfg.keycodef04, KeyCodeFXX[3]);
////	tranKeyCode(cfg.keycodef05, KeyCodeFXX[4]);
////	tranKeyCode(cfg.keycodef06, KeyCodeFXX[5]);
////	tranKeyCode(cfg.keycodef07, KeyCodeFXX[6]);
////	tranKeyCode(cfg.keycodef08, KeyCodeFXX[7]);
////	tranKeyCode(cfg.keycodef09, KeyCodeFXX[8]);
////	tranKeyCode(cfg.keycodef10, KeyCodeFXX[9]);
////	tranKeyCode(cfg.keycodef11, KeyCodeFXX[10]);
////	tranKeyCode(cfg.keycodef12, KeyCodeFXX[11]);
//
////}
//
////static int CALLBACK UserKeyProc(HWND hwnd, UINT msg, WPARAM wParam,
////		LPARAM lParam) {
////	char keyValue[8];
////	int rv = 0;
////	switch (msg) {
////	case WM_INITDIALOG:
////		keypro_init(hwnd);
////		return 1;
////	case WM_COMMAND:
////		switch (LOWORD(wParam)) {
////		case IDC_RESUMVALUE:
////			if (MessageBox(hwnd, SURE_REVERT_DEFAULT, TITLE_POINT,
////					MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK) {
////				keypro_resumvalue();
////				keypro_init(hwnd);
////			}
////			break;
////		case IDOK:
////			dlg_to_cfg(hwnd);
////			save_settings(sectionName, &cfg);
////			set_cfgto_uchar();
////			MessageBox(hwnd, SAVE_SET_CUSTOM, TITLE_POINT,
////					MB_ICONINFORMATION | MB_OK | MB_DEFBUTTON1);
////			SendMessage(GetDlgItem(hwnd, IDC_IPADDRESS), CB_SETCURSEL, -1, 0);
////			SetDlgItemText(hwnd, IDC_EDITPORT, cfg.port);
////		case IDCANCEL:
////			EndDialog(hwnd, TRUE);
////			break;
////		}
////		return 0;
////	case WM_CLOSE:
////		EndDialog(hwnd, TRUE);
////		return 0;
////	}
////	return 0;
////}
////
////void ShowUserKey(HWND hwnd) {
////	DialogBox(hinst, MAKEINTRESOURCE(IDD_USERKEY), hwnd, UserKeyProc);
////}
//
//static int CALLBACK LogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	int i;
//
//	switch (msg) {
//	case WM_INITDIALOG: {
//		char *str = dupprintf("%s Event Log", appname);
//		SetWindowText(hwnd, str);
//		sfree(str);
//	}
//		{
//			static int tabs[4] = { 78, 108 };
//			SendDlgItemMessage(hwnd, IDN_LIST, LB_SETTABSTOPS, 2,
//					(LPARAM) tabs);
//		}
//		for (i = 0; i < nevents; i++)
//			SendDlgItemMessage(hwnd, IDN_LIST, LB_ADDSTRING, 0,
//					(LPARAM) events[i]);
//		return 1;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDOK:
//		case IDCANCEL:
//			logbox = NULL;
//			SetActiveWindow(GetParent(hwnd));
//			DestroyWindow(hwnd);
//			return 0;
//		case IDN_COPY:
//			if (HIWORD(wParam) == BN_CLICKED
//					|| HIWORD(wParam) == BN_DOUBLECLICKED) {
//				int selcount;
//				int *selitems;
//				selcount = SendDlgItemMessage(hwnd, IDN_LIST, LB_GETSELCOUNT, 0,
//						0);
//				if (selcount == 0) { /* don't even try to copy zero items */
//					MessageBeep(0);
//					break;
//				}
//
//				selitems = snewn(selcount, int);
//				if (selitems) {
//					int count = SendDlgItemMessage(hwnd, IDN_LIST,
//							LB_GETSELITEMS, selcount, (LPARAM) selitems);
//					int i;
//					int size;
//					char *clipdata;
//					static unsigned char sel_nl[] = SEL_NL;
//
//					if (count == 0) { /* can't copy zero stuff */
//						MessageBeep(0);
//						break;
//					}
//
//					size = 0;
//					for (i = 0; i < count; i++)
//						size += strlen(events[selitems[i]]) + sizeof(sel_nl);
//
//					clipdata = snewn(size, char);
//					if (clipdata) {
//						char *p = clipdata;
//						for (i = 0; i < count; i++) {
//							char *q = events[selitems[i]];
//							int qlen = strlen(q);
//							memcpy(p, q, qlen);
//							p += qlen;
//							memcpy(p, sel_nl, sizeof(sel_nl));
//							p += sizeof(sel_nl);
//						}
//						write_aclip(NULL, clipdata, size, TRUE);
//						sfree(clipdata);
//					}
//					sfree(selitems);
//
//					for (i = 0; i < nevents; i++)
//						SendDlgItemMessage(hwnd, IDN_LIST, LB_SETSEL, FALSE, i);
//				}
//			}
//			return 0;
//		}
//		return 0;
//	case WM_CLOSE:
//		logbox = NULL;
//		SetActiveWindow(GetParent(hwnd));
//		DestroyWindow(hwnd);
//		return 0;
//	}
//	return 0;
//}
//
//static int CALLBACK LicenceProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//	switch (msg) {
//	case WM_INITDIALOG: {
//		char *str = dupprintf("%s", appname);
//		SetWindowText(hwnd, str);
//		sfree(str);
//	}
//		return 1;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDOK:
//		case IDCANCEL:
//			EndDialog(hwnd, TRUE);
//			return 0;
//		}
//		return 0;
//	case WM_CLOSE:
//		EndDialog(hwnd, 1);
//		return 0;
//	}
//	return 0;
//}
//
//static int CALLBACK AboutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
//	char *str;
//	static HBITMAP hBmp;
//
//	switch (msg) {
//	case WM_CREATE:
//		break;
//	case WM_PAINT:
//		break;
//	case WM_INITDIALOG:
//		hBmp = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP1)); //加载位图
//
//		SendDlgItemMessage(hwnd, //   对话框句柄
//				IDC_PIC, //   图片控件ID
//				STM_SETIMAGE, //   设置消息
//				(WPARAM) IMAGE_BITMAP, //   图象类型
//				(LPARAM) hBmp); //   位图句柄
//		return 0;
//	case WM_DESTROY:
//		break;
//	case WM_COMMAND:
//		switch (LOWORD(wParam)) {
//		case IDOK:
//		case IDCANCEL:
//			EndDialog(hwnd, TRUE);
//			return 0;
//		case IDA_LICENCE:
//
//			return 0;
//
//		case IDA_WEB:
//			/* Load web browser */
//			ShellExecute(hwnd, "open", "http://www.huayoutech.com/", 0, 0,
//					SW_SHOWDEFAULT);
//			return 0;
//		}
//		return 0;
//	case WM_CLOSE:
//		EndDialog(hwnd, TRUE);
//		return 0;
//	}
//	return 0;
//}

//static int SaneDialogBox(HINSTANCE hinst, LPCTSTR tmpl, HWND hwndparent,
//		DLGPROC lpDialogFunc) {
//	WNDCLASS wc;
//	HWND hwnd;
//	MSG msg;
//	int flags;
//	int ret;
//	int gm;
//
//	wc.style = CS_DBLCLKS | CS_SAVEBITS | CS_BYTEALIGNWINDOW;
//	wc.lpfnWndProc = DefDlgProc;
//	wc.cbClsExtra = 0;
//	wc.cbWndExtra = DLGWINDOWEXTRA + 2 * sizeof(LONG_PTR);
//	wc.hInstance = hinst;
//	wc.hIcon = NULL;
//	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
//	wc.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
//	wc.lpszMenuName = NULL;
//	wc.lpszClassName = "PuTTYConfigBox";
//	RegisterClass(&wc);
//
//	hwnd = CreateDialog(hinst, tmpl, hwndparent, lpDialogFunc);
//
//	SetWindowLongPtr(hwnd, BOXFLAGS, 0); /* flags */
//	SetWindowLongPtr(hwnd, BOXRESULT, 0); /* result from SaneEndDialog */
//
//	while ((gm = GetMessage(&msg, NULL, 0, 0)) > 0) {
//		flags = GetWindowLongPtr(hwnd, BOXFLAGS);
//		if (!(flags & DF_END) && !IsDialogMessage(hwnd, &msg))
//			DispatchMessage(&msg);
//		if (flags & DF_END)
//			break;
//	}
//
//	if (gm == 0)
//		PostQuitMessage(msg.wParam); /* We got a WM_QUIT, pass it on */
//
//	ret = GetWindowLongPtr(hwnd, BOXRESULT);
//	DestroyWindow(hwnd);
//	return ret;
//}
//
//static void SaneEndDialog(HWND hwnd, int ret) {
//	SetWindowLongPtr(hwnd, BOXRESULT, ret);
//	SetWindowLongPtr(hwnd, BOXFLAGS, DF_END);
//}
//
///*
// * Null dialog procedure.
// */
//static int CALLBACK NullDlgProc(HWND hwnd, UINT msg, WPARAM wParam,
//		LPARAM lParam) {
//	return 0;
//}
//
//enum {
//	IDCX_ABOUT = IDC_ABOUT,
//	IDCX_TVSTATIC,
//	IDCX_TREEVIEW,
//	IDCX_STDBASE,
//	IDCX_PANELBASE = IDCX_STDBASE + 32
//};
//
//struct treeview_faff {
//	HWND treeview;
//	HTREEITEM lastat[4];
//};
//
//static HTREEITEM treeview_insert(struct treeview_faff *faff, int level,
//		char *text, char *path) {
//	TVINSERTSTRUCT ins;
//	int i;
//	HTREEITEM newitem;
//	ins.hParent = (level > 0 ? faff->lastat[level - 1] : TVI_ROOT);
//	ins.hInsertAfter = faff->lastat[level];
//#if _WIN32_IE >= 0x0400 && defined NONAMELESSUNION
//#define INSITEM DUMMYUNIONNAME.item
//#else
//#define INSITEM item
//#endif
//	ins.INSITEM.mask = TVIF_TEXT | TVIF_PARAM;
//	ins.INSITEM.pszText = text;
//	ins.INSITEM.cchTextMax = strlen(text) + 1;
//	ins.INSITEM.lParam = (LPARAM) path;
//	newitem = TreeView_InsertItem(faff->treeview, &ins);
//	if (level > 0)
//		TreeView_Expand(faff->treeview, faff->lastat[level - 1],
//				(level > 1 ? TVE_COLLAPSE : TVE_EXPAND));
//	faff->lastat[level] = newitem;
//	for (i = level + 1; i < 4; i++)
//		faff->lastat[i] = NULL;
//	return newitem;
//}
//
///*
// * Create the panelfuls of controls in the configuration box.
// */
////static void create_controls(HWND hwnd, char *path) {
////	struct ctlpos cp;
////	int index;
////	int base_id;
////	struct winctrls *wc;
////
////	if (!path[0]) {
////		/*
////		 * Here we must create the basic standard controls.
////		 */
////		ctlposinit(&cp, hwnd, 3, 3, 235);
////		wc = &ctrls_base;
////		base_id = IDCX_STDBASE;
////	} else {
////		/*
////		 * Otherwise, we're creating the controls for a particular
////		 * panel.
////		 */
////		ctlposinit(&cp, hwnd, 100, 3, 13);
////		wc = &ctrls_panel;
////		base_id = IDCX_PANELBASE;
////	}
////
////	for (index = -1; (index = ctrl_find_path(ctrlbox, path, index)) >= 0;) {
////		struct controlset *s = ctrlbox->ctrlsets[index];
////		winctrl_layout(&dp, wc, &cp, s, &base_id);
////	}
////}
///*
// * This function is the configuration box.
// * (Being a dialog procedure, in general it returns 0 if the default
// * dialog processing should be performed, and 1 if it should not.)
// */
////static int CALLBACK GenericMainDlgProc(HWND hwnd, UINT msg, WPARAM wParam,
////		LPARAM lParam) {
////	HWND hw, treeview;
////	struct treeview_faff tvfaff;
////	int ret;
////
////	switch (msg) {
////	case WM_INITDIALOG:
////		dp.hwnd = hwnd;
////		create_controls(hwnd, ""); /* Open and Cancel buttons etc */
////		SetWindowText(hwnd, dp.wintitle);
////		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
////		if (has_help())
////			SetWindowLongPtr(hwnd, GWL_EXSTYLE,
////					GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);
////		else {
////			HWND item = GetDlgItem(hwnd, IDC_HELPBTN);
////			if (item)
////				DestroyWindow(item);
////		}
////		SendMessage(hwnd, WM_SETICON, (WPARAM) ICON_BIG,
////				(LPARAM) LoadIcon(hinst, MAKEINTRESOURCE(IDI_CFGICON)));
////		/*
////		 * Centre the window.
////		 */
////		{ /* centre the window */
////			RECT rs, rd;
////
////			hw = GetDesktopWindow();
////			if (GetWindowRect(hw, &rs) && GetWindowRect(hwnd, &rd))
////				MoveWindow(hwnd, (rs.right + rs.left + rd.left - rd.right) / 2,
////						(rs.bottom + rs.top + rd.top - rd.bottom) / 2,
////						rd.right - rd.left, rd.bottom - rd.top, TRUE);
////		}
////
////		/*
////		 * Create the tree view.
////		 */
////		{
////			RECT r;
////			WPARAM font;
////			HWND tvstatic;
////
////			r.left = 3;
////			r.right = r.left + 95;
////			r.top = 3;
////			r.bottom = r.top + 10;
////			MapDialogRect(hwnd, &r);
////			tvstatic = CreateWindowEx(0, "STATIC", "Cate&gory:",
////					WS_CHILD | WS_VISIBLE, r.left, r.top, r.right - r.left,
////					r.bottom - r.top, hwnd, (HMENU) IDCX_TVSTATIC, hinst, NULL);
////			font = SendMessage(hwnd, WM_GETFONT, 0, 0);
////			SendMessage(tvstatic, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
////
////			r.left = 3;
////			r.right = r.left + 95;
////			r.top = 13;
////			r.bottom = r.top + 219;
////			MapDialogRect(hwnd, &r);
////			treeview = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
////					WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_HASLINES
////							| TVS_DISABLEDRAGDROP | TVS_HASBUTTONS
////							| TVS_LINESATROOT | TVS_SHOWSELALWAYS, r.left,
////					r.top, r.right - r.left, r.bottom - r.top, hwnd,
////					(HMENU) IDCX_TREEVIEW, hinst, NULL);
////			font = SendMessage(hwnd, WM_GETFONT, 0, 0);
////			SendMessage(treeview, WM_SETFONT, font, MAKELPARAM(TRUE, 0));
////			tvfaff.treeview = treeview;
////			memset(tvfaff.lastat, 0, sizeof(tvfaff.lastat));
////		}
////
////		/*
////		 * Set up the tree view contents.
////		 */
////		{
////			HTREEITEM hfirst = NULL;
////			int i;
////			char *path = NULL;
////
////			for (i = 0; i < ctrlbox->nctrlsets; i++) {
////				struct controlset *s = ctrlbox->ctrlsets[i];
////				HTREEITEM item;
////				int j;
////				char *c;
////
////				if (!s->pathname[0])
////					continue;
////				j = path ? ctrl_path_compare(s->pathname, path) : 0;
////				if (j == INT_MAX)
////					continue; /* same path, nothing to add to tree */
////
////				/*
////				 * We expect never to find an implicit path
////				 * component. For example, we expect never to see
////				 * A/B/C followed by A/D/E, because that would
////				 * _implicitly_ create A/D. All our path prefixes
////				 * are expected to contain actual controls and be
////				 * selectable in the treeview; so we would expect
////				 * to see A/D _explicitly_ before encountering
////				 * A/D/E.
////				 */
////				if (!(j == ctrl_path_elements(s->pathname) - 1)) {
////					return 0; //zhangbo
////				}
////				//assert(j == ctrl_path_elements(s->pathname) - 1);
////
////				c = strrchr(s->pathname, '/');
////				if (!c)
////					c = s->pathname;
////				else
////					c++;
////
////				item = treeview_insert(&tvfaff, j, c, s->pathname);
////				if (!hfirst)
////					hfirst = item;
////
////				path = s->pathname;
////			}
////
////			/*
////			 * Put the treeview selection on to the Session panel.
////			 * This should also cause creation of the relevant
////			 * controls.
////			 */
////			TreeView_SelectItem(treeview, hfirst);
////		}
////
////		/*
////		 * Set focus into the first available control.
////		 */
////		{
////			int i;
////			struct winctrl *c;
////
////			for (i = 0; (c = winctrl_findbyindex(&ctrls_panel, i)) != NULL;
////					i++) {
////				if (c->ctrl) {
////					dlg_set_focus(c->ctrl, &dp);
////					break;
////				}
////			}
////		}
////
////		SetWindowLongPtr(hwnd, GWLP_USERDATA, 1);
////		return 0;
////	case WM_LBUTTONUP:
////		/*
////		 * Button release should trigger WM_OK if there was a
////		 * previous double click on the session list.
////		 */
////		ReleaseCapture();
////		if (dp.ended)
////			SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
////		break;
////	case WM_NOTIFY:
////		if (LOWORD(wParam) == IDCX_TREEVIEW&&
////		((LPNMHDR) lParam)->code == TVN_SELCHANGED) {HTREEITEM i =
////		TreeView_GetSelection(((LPNMHDR) lParam)->hwndFrom);
////		TVITEM item;
////		char buffer[64];
////
////		SendMessage (hwnd, WM_SETREDRAW, FALSE, 0);
////
////		item.hItem = i;
////		item.pszText = buffer;
////		item.cchTextMax = sizeof(buffer);
////		item.mask = TVIF_TEXT | TVIF_PARAM;
////		TreeView_GetItem(((LPNMHDR) lParam)->hwndFrom, &item);
////		{
////			/* Destroy all controls in the currently visible panel. */
////			int k;
////			HWND item;
////			struct winctrl *c;
////
////			while ((c = winctrl_findbyindex(&ctrls_panel, 0)) != NULL) {
////				for (k = 0; k < c->num_ids; k++) {
////					item = GetDlgItem(hwnd, c->base_id + k);
////					if (item)
////					DestroyWindow(item);
////				}
////				winctrl_rem_shortcuts(&dp, c);
////				winctrl_remove(&ctrls_panel, c);
////				sfree(c->data);
////				sfree(c);
////			}
////		}
////		create_controls(hwnd, (char *)item.lParam);
////
////		dlg_refresh(NULL, &dp); /* set up control values */
////
////		SendMessage (hwnd, WM_SETREDRAW, TRUE, 0);
////		InvalidateRect (hwnd, NULL, TRUE);
////
////		SetFocus(((LPNMHDR) lParam)->hwndFrom); /* ensure focus stays */
////		return 0;
////	}
////	break;
////	case WM_COMMAND:
////	case WM_DRAWITEM:
////	default: /* also handle drag list msg here */
////	/*
////	 * Only process WM_COMMAND once the dialog is fully formed.
////	 */
////	if (GetWindowLongPtr(hwnd, GWLP_USERDATA) == 1) {
////		ret = winctrl_handle_command(&dp, msg, wParam, lParam);
////		if (dp.ended && GetCapture() != hwnd)
////		SaneEndDialog(hwnd, dp.endresult ? 1 : 0);
////	} else
////	ret = 0;
////	return ret;
////	case WM_HELP:
////	if (!winctrl_context_help(&dp, hwnd,
////					((LPHELPINFO)lParam)->iCtrlId))
////	MessageBeep(0);
////	break;
////	case WM_CLOSE:
////	quit_help(hwnd);
////	SaneEndDialog(hwnd, 0);
////	return 0;
////
////	/* Grrr Explorer will maximize Dialogs! */
////	case WM_SIZE:
////	if (wParam == SIZE_MAXIMIZED)
////	force_normal(hwnd);
////	return 0;
////
////}
////	return 0;
////}
//void modal_about_box(HWND hwnd) {
//	EnableWindow(hwnd, 0);
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
//	EnableWindow(hwnd, 1);
//	SetActiveWindow(hwnd);
//}
//
//void show_help(HWND hwnd) {
//	launch_help(hwnd, NULL);
//}
//
//void defuse_showwindow(void) {
//	/*
//	 * Work around the fact that the app's first call to ShowWindow
//	 * will ignore the default in favour of the shell-provided
//	 * setting.
//	 */
//	{
//		HWND hwnd;
//		hwnd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX),
//				NULL, NullDlgProc);
//		ShowWindow(hwnd, SW_HIDE);
//		SetActiveWindow(hwnd);
//		DestroyWindow(hwnd);
//	}
//}
//
////int do_config(void) {
////	int ret;
////
////	ctrlbox = ctrl_new_box();
////	setup_config_box(ctrlbox, FALSE, 0, 0);
////	win_setup_config_box(ctrlbox, &dp.hwnd, has_help(), FALSE, 0);
////	dp_init(&dp);
////	winctrl_init(&ctrls_base);
////	winctrl_init(&ctrls_panel);
////	dp_add_tree(&dp, &ctrls_base);
////	dp_add_tree(&dp, &ctrls_panel);
////	dp.wintitle = dupprintf("%s Configuration", appname);
////	dp.errtitle = dupprintf("%s Error", appname);
////	dp.data = &cfg;
////	dp.shortcuts['g'] = TRUE; /* the treeview: `Cate&gory' */
////
////	ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_MAINBOX), NULL,
////			GenericMainDlgProc);
////
////	ctrl_free_box(ctrlbox);
////	winctrl_cleanup(&ctrls_panel);
////	winctrl_cleanup(&ctrls_base);
////	dp_cleanup(&dp);
////
////	return ret;
////}
//
////int do_reconfig(HWND hwnd, int protcfginfo) {
////	Config backup_cfg;
////	int ret;
////
////	backup_cfg = cfg; /* structure copy */
////
////	ctrlbox = ctrl_new_box();
////	setup_config_box(ctrlbox, TRUE, cfg.protocol, protcfginfo);
////	win_setup_config_box(ctrlbox, &dp.hwnd, has_help(), TRUE, cfg.protocol);
////	dp_init(&dp);
////	winctrl_init(&ctrls_base);
////	winctrl_init(&ctrls_panel);
////	dp_add_tree(&dp, &ctrls_base);
////	dp_add_tree(&dp, &ctrls_panel);
////	dp.wintitle = dupprintf("%s Reconfiguration", appname);
////	dp.errtitle = dupprintf("%s Error", appname);
////	dp.data = &cfg;
////	dp.shortcuts['g'] = TRUE; /* the treeview: `Cate&gory' */
////
////	ret = SaneDialogBox(hinst, MAKEINTRESOURCE(IDD_MAINBOX), NULL,
////			GenericMainDlgProc);
////
////	ctrl_free_box(ctrlbox);
////	winctrl_cleanup(&ctrls_base);
////	winctrl_cleanup(&ctrls_panel);
////	dp_cleanup(&dp);
////
////	if (!ret)
////		cfg = backup_cfg; /* structure copy */
////
////	return ret;
////}
//
void logevent(void *frontend, const char *string) {
	char timebuf[40];
	struct tm tm;

	log_eventlog(logctx, string);

	if (nevents >= negsize) {
		negsize += 64;
		events = sresize(events, negsize, char *);
	}

	tm = ltime();
	strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S\t", &tm);

	events[nevents] = snewn(strlen(timebuf) + strlen(string) + 1, char);
	strcpy(events[nevents], timebuf);
	strcat(events[nevents], string);
	if (logbox) {
		int count;
		SendDlgItemMessage(logbox, IDN_LIST, LB_ADDSTRING, 0,
				(LPARAM) events[nevents]);
		count = SendDlgItemMessage(logbox, IDN_LIST, LB_GETCOUNT, 0, 0);
		SendDlgItemMessage(logbox, IDN_LIST, LB_SETTOPINDEX, count - 1, 0);
	}
	nevents++;
}
//
//void showeventlog(HWND hwnd) {
//	if (!logbox) {
//		logbox = CreateDialog(hinst, MAKEINTRESOURCE(IDD_LOGBOX),
//				hwnd, LogProc);
//		ShowWindow(logbox, SW_SHOWNORMAL);
//	}
//	SetActiveWindow(logbox);
//}
//
//void showabout(HWND hwnd) {
//	DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, AboutProc);
//}
//
//int verify_ssh_host_key(void *frontend, char *host, int port, char *keytype,
//		char *keystr, char *fingerprint,
//		void (*callback)(void *ctx, int result), void *ctx) {
//	int ret;
//
//	static const char absentmsg[] =
//			"The server's host key is not cached in the registry. You\n"
//					"have no guarantee that the server is the computer you\n"
//					"think it is.\n"
//					"The server's %s key fingerprint is:\n"
//					"%s\n"
//					"If you trust this host, hit Yes to add the key to\n"
//					"%s's cache and carry on connecting.\n"
//					"If you want to carry on connecting just once, without\n"
//					"adding the key to the cache, hit No.\n"
//					"If you do not trust this host, hit Cancel to abandon the\n"
//					"connection.\n";
//
//	static const char wrongmsg[] = "WARNING - POTENTIAL SECURITY BREACH!\n"
//			"\n"
//			"The server's host key does not match the one %s has\n"
//			"cached in the registry. This means that either the\n"
//			"server administrator has changed the host key, or you\n"
//			"have actually connected to another computer pretending\n"
//			"to be the server.\n"
//			"The new %s key fingerprint is:\n"
//			"%s\n"
//			"If you were expecting this change and trust the new key,\n"
//			"hit Yes to update %s's cache and continue connecting.\n"
//			"If you want to carry on connecting but without updating\n"
//			"the cache, hit No.\n"
//			"If you want to abandon the connection completely, hit\n"
//			"Cancel. Hitting Cancel is the ONLY guaranteed safe\n" "choice.\n";
//
//	static const char mbtitle[] = "%s Security Alert";
//
//	/*
//	 * Verify the key against the registry.
//	 */
//	ret = verify_host_key(host, port, keytype, keystr);
//
//	if (ret == 0) /* success - key matched OK */
//		return 1;
//	if (ret == 2) { /* key was different */
//		int mbret;
//		char *text = dupprintf(wrongmsg, appname, keytype, fingerprint,
//				appname);
//		char *caption = dupprintf(mbtitle, appname);
//		mbret = message_box(text, caption,
//				MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3,
//				HELPCTXID(errors_hostkey_changed));
//		if (!(mbret == IDYES || mbret == IDNO || mbret == IDCANCEL)) {
//			return 1;
//		}
//		//assert(mbret==IDYES || mbret==IDNO || mbret==IDCANCEL);
//		sfree(text);
//		sfree(caption);
//		if (mbret == IDYES) {
//			store_host_key(host, port, keytype, keystr);
//			return 1;
//		} else if (mbret == IDNO)
//			return 1;
//		return 0;
//	}
//	if (ret == 1) { /* key was absent */
//		int mbret;
//		char *text = dupprintf(absentmsg, keytype, fingerprint, appname);
//		char *caption = dupprintf(mbtitle, appname);
//		mbret = message_box(text, caption,
//				MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON3,
//				HELPCTXID(errors_hostkey_absent));
//		if (!(mbret == IDYES || mbret == IDNO || mbret == IDCANCEL)) {
//			return 1;
//		}
//		//assert(mbret==IDYES || mbret==IDNO || mbret==IDCANCEL);
//		sfree(text);
//		sfree(caption);
//		if (mbret == IDYES) {
//			store_host_key(host, port, keytype, keystr);
//			return 1;
//		} else if (mbret == IDNO)
//			return 1;
//		return 0;
//	}
//}
//
///*
// * Ask whether the selected algorithm is acceptable (since it was
// * below the configured 'warn' threshold).
// */
//int askalg(void *frontend, const char *algtype, const char *algname,
//		void (*callback)(void *ctx, int result), void *ctx) {
//	static const char mbtitle[] = "%s Security Alert";
//	static const char msg[] = "The first %s supported by the server\n"
//			"is %.64s, which is below the configured\n"
//			"warning threshold.\n"
//			"Do you want to continue with this connection?\n";
//	char *message, *title;
//	int mbret;
//
//	message = dupprintf(msg, algtype, algname);
//	title = dupprintf(mbtitle, appname);
//	mbret = MessageBox(NULL, message, title,
//			MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
//	socket_reselect_all();
//	sfree(message);
//	sfree(title);
//	if (mbret == IDYES)
//		return 1;
//	else
//		return 0;
//}
//
///*
// * Ask whether to wipe a session log file before writing to it.
// * Returns 2 for wipe, 1 for append, 0 for cancel (don't log).
// */
int askappend(void *frontend, Filename filename,
		void (*callback)(void *ctx, int result), void *ctx) {
	static const char msgtemplate[] =
			"The session log file \"%.*s\" already exists.\n"
					"You can overwrite it with a new session log,\n"
					"append your session log to the end of it,\n"
					"or disable session logging for this session.\n"
					"Hit Yes to wipe the file, No to append to it,\n"
					"or Cancel to disable logging.";
	char *message;
	char *mbtitle;
	int mbret;

	message = dupprintf(msgtemplate, FILENAME_MAX, filename.path);
	mbtitle = dupprintf("%s Log to File", appname);

	mbret = MessageBox(NULL, message, mbtitle,
			MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON3);

	socket_reselect_all();

	sfree(message);
	sfree(mbtitle);

	if (mbret == IDYES)
		return 2;
	else if (mbret == IDNO)
		return 1;
	else
		return 0;
}

///*
// * Warn about the obsolescent key file format.
// *
// * Uniquely among these functions, this one does _not_ expect a
// * frontend handle. This means that if PuTTY is ported to a
// * platform which requires frontend handles, this function will be
// * an anomaly. Fortunately, the problem it addresses will not have
// * been present on that platform, so it can plausibly be
// * implemented as an empty function.
// */
//void old_keyfile_warning(void) {
//	static const char mbtitle[] = "%s Key File Warning";
//	static const char message[] =
//			"You are loading an SSH-2 private key which has an\n"
//					"old version of the file format. This means your key\n"
//					"file is not fully tamperproof. Future versions of\n"
//					"%s may stop supporting this private key format,\n"
//					"so we recommend you convert your key to the new\n"
//					"format.\n"
//					"\n"
//					"You can perform this conversion by loading the key\n"
//					"into PuTTYgen and then saving it again.";
//
//	char *msg, *title;
//	msg = dupprintf(message, appname);
//	title = dupprintf(mbtitle, appname);
//
//	MessageBox(NULL, msg, title, MB_OK);
//
//	socket_reselect_all();
//
//	sfree(msg);
//	sfree(title);
//}
