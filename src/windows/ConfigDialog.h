/*
 * ConfigDialog.h
 *
 *  Created on: 2012-9-25
 *      Author: xia
 */

#ifndef CONFIGDIALOG_H_
#define CONFIGDIALOG_H_
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
#include "windef.h"
#include "winuser.h"

namespace terminal {

void LoadConfigAndProperties();

class ConfigDialog {
private:

public:
	ConfigDialog();
	virtual ~ConfigDialog();
	static void ShowInterface(HWND hwnd);
	static void set_cfgto_uchar();
	static void ShowReg(HWND hwnd);
	static void ShowSet(HWND hwnd);
	static int GetComSet(HWND hwnd, int ID, int add);
	static void AddComCB(HWND hwnd, char*pCOMNumber);
	static void InitCom(HWND hwnd);
	static void InitTabSheet0(HWND hwnd);
	static void InitTabSheet2(HWND hwnd);
	static void ShowNewTerm(HWND hwnd);
	static void SaveTabSheet(int iCurrentpage);
	static void ShowAdmin(HWND hwnd);
	static void ShowHelp(HWND hwnd);
	static void ShowSetComName(HWND hwnd);

};

} /* namespace terminal */
#endif /* CONFIGDIALOG_H_ */
