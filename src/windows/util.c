/*
 * util.c
 *
 *  Created on: 2013年11月30日
 *      Author: zhangbo
 */

#include "util.h"

void MoveToCenter(HWND hwnd) {
	RECT rcDlg;
	HWND hWndParent;
	GetWindowRect(hwnd, &rcDlg);
	RECT rcParent;
	hWndParent = GetParent(hwnd);
	GetClientRect(hWndParent, &rcParent);
	POINT ptParentInScreen;
	ptParentInScreen.x = rcParent.left;
	ptParentInScreen.y = rcParent.top;
	ClientToScreen(hWndParent, (LPPOINT) &ptParentInScreen);
	SetWindowPos(hwnd, NULL,
			ptParentInScreen.x
					+ (rcParent.right - rcParent.left
							- (rcDlg.right - rcDlg.left)) / 2,
			ptParentInScreen.y
					+ (rcParent.bottom - rcParent.top
							- (rcDlg.bottom - rcDlg.top)) / 2, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
}
