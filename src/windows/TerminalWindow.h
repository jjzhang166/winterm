/*
 * TerminalWindow.h
 *
 *  Created on: 2012-9-13
 *      Author: zhangbo
 */

//#ifdef __cplusplus
//extern "C" {
//#endif
#ifndef TERMINALWINDOW_H_
#define TERMINALWINDOW_H_

#include <windows.h>
#include "../putty.h"

namespace terminal {

class TerminalWindow {
private:
	static HHOOK g_hKeyBoard;
	static HHOOK g_hKeyMouse;

	static HMENU savedsess_menu;
	static UINT wm_mousewheel;
	static int wheel_accumulator;
	static int ignore_clip;
	static int need_backend_resize;
	static int fullscr_on_max;
	static UINT last_mousemove;
	//接收所有窗口消息
	static int isShowMenu; //1为显示菜单,0为隐藏菜单
	static int ishaveMenu; //0原来没有菜单栏,1原来有菜单栏
	static int myIsFullSC; //是否全屏幕显示
	static int dbltime, lasttime;
	static Mouse_Action lastact;
	static Mouse_Button lastbtn;
	static int resizing;
	static int was_zoomed;
	static int prev_rows, prev_cols;
	static char *buffer;
	static long timing_next_time;
	static void *ldisc;
	static int mouseX;
	static int mouseY;
	static WPARAM pend_netevent_wParam;
	static LPARAM pend_netevent_lParam;
	static int pending_netevent;
	static int kbd_codepage;
	static int show;
	static HINSTANCE prev;
	static HBITMAP caretbm;
	static int compose_state;
	static struct sesslist sesslist;

	enum {
		SYSMENU, CTXMENU
	};

	static int is_alt_pressed(void);
	static Mouse_Button translate_button(Mouse_Button button);
	static void click(Mouse_Button b, int x, int y, int shift, int ctrl,
			int alt);
	static void show_mouseptr(int show);
	static void update_savedsess_menu(void);
	static void update_specials_menu(void *frontend);
	static void update_mouse_pointer(void);
	static LRESULT CALLBACK KeyBoardProc(int code, // hook code
			WPARAM wParam, // virtual-key code
			LPARAM lParam // keystroke-message information
			);
	static LRESULT CALLBACK KeyMouseProc(int code, // hook code
			WPARAM wParam, // virtual-key code
			LPARAM lParam // keystroke-message information
			);

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam,
			LPARAM lParam);
	static bool RegistSuccess();
	static bool Single(void * hMutex, LPCSTR name);
	static void cfgtopalette(void);
	static void set_input_locale(HKL kl);
	static void init_palette(void);
	static void start_backend(void);
	static void close_session(void);
	static void enact_pending_netevent(void);
	static int TranslateKey(UINT message, WPARAM wParam, LPARAM lParam,
			unsigned char *output);
protected:
	//xiahui
	static void MonitorCom(char* title);
	static LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	static LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
	static LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);
	static LRESULT OnClose(WPARAM wParam, LPARAM lParam);
	static LRESULT OnInitMenupopup(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnCommand(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnExitSizeMove(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnButtonDownUp(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnSetFocus(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnMouseMove(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnNcMouseMove(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnPaint(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnNetEvent(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnKillFocus(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnSizing(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnSize(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnVscroll(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnPaletteChangerd(UINT message, WPARAM wParam,
			LPARAM lParam);
	static BOOL OnQueryNewpalette();

	//zhangbo
	static LRESULT OnKeyDown(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnSysKeyUp(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnInputLangChange(UINT message, WPARAM wParam,
			LPARAM lParam);
	static LRESULT OnImeStartComposition(UINT message, WPARAM wParam,
			LPARAM lParam);
	static LRESULT OnImeEndComposition(UINT message, WPARAM wParam,
			LPARAM lParam);
	static LRESULT OnImeComposition(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnImeChar(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnChar(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnSysColorChange(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnAgentCallback(UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT OnMouseWheel(UINT message, WPARAM wParam, LPARAM lParam);
	static void GenerateMach(char * localRegistCod, int type);

public:
	TerminalWindow(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show);
	virtual ~TerminalWindow();
	static void sendpassword();
	static void Run(HINSTANCE inst); //, const char* name
};

} /* namespace term */
#endif /* TERMINALWINDOW_H_ */

//#ifdef __cplusplus
//}
//#endif
