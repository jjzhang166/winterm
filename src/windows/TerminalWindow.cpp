/*
 * TerminalWindow.cpp
 *
 *  Created on: 2012-9-13
 *      Author: zhangbo
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "window.h"
#include <util/log.h>

#include "../resources/resource.h"
#include "TerminalWindow.h"
#include "message.h"
#include "../terminal.h"
#include "../ldisc.h"
#include "../ldiscucs.h"
#include "../getPcMsgApi.h"
#include <util/encrypt.h>
#include "../putty.h"
#include <cpputils/Util.h>
#include <iostream>
#include <cpputils/ThreadCreator.h>
#include "../pairs.h"
#include <cpputils/Properties.h>
#include "../Setting.h"
#include <util/comdev.h>

//#define ZHANGBO_DEBUG

#ifdef ZHANGBO_DEBUG
#define DEBUG_MAIN
#define DEBUG_CAN_INPUT
#endif

extern "C" {
extern char comdev_char;
extern BOOLEAN comdev_reading;
char localRegistKey[1024];
char sectionName[256] = "default";
extern int AllLock;
extern int font_width, font_height, font_dualwidth;
extern int extra_width, extra_height;
extern int offset_width, offset_height;
extern COLORREF colours[NALLCOLOURS];
extern HPALETTE pal;
extern LPLOGPALETTE logpal;
extern RGBTRIPLE defpal[NALLCOLOURS];
extern int IsPrintScreen;
extern char *window_name, *icon_name;
extern Backend *back;
extern int busy_status;
extern int send_raw_mouse;
extern void *backhandle;
extern int caret_x;
extern int caret_y;
extern int must_close_session, session_closed;
extern unsigned char KeyCodeFXX[12][KEYCODEFXX_MAX_SIZE / 2];
extern int KeyCodeLenFXX[12];
extern const struct telnet_special *specials;
extern HMENU specials_menu;
extern int n_specials;
extern LOGFONT lfont;
extern struct unicode_data ucsdata;
extern struct _popup_menus popup_menus[2];

int tranKeyCode(char *keyStr, unsigned char *keyCodeStr);
int get_fullscreen_rect(RECT * ss);
void init_fonts(int pick_width, int pick_height);
void systopalette(void);

}

namespace terminal {
UINT TerminalWindow::wm_mousewheel = WM_MOUSEWHEEL;
int TerminalWindow::wheel_accumulator = 0;
int TerminalWindow::ignore_clip = FALSE;
int TerminalWindow::need_backend_resize = FALSE;
int TerminalWindow::fullscr_on_max = FALSE;
UINT TerminalWindow::last_mousemove = 0;
int TerminalWindow::isShowMenu = 1; //1为显示菜单,0为隐藏菜单
int TerminalWindow::ishaveMenu = 1; //0原来没有菜单栏,1原来有菜单栏
int TerminalWindow::myIsFullSC = 0; //是否全屏幕显示
int TerminalWindow::was_zoomed = 0;
char* TerminalWindow::buffer = NULL;
int TerminalWindow::compose_state = 0;
int TerminalWindow::resizing = 0;
void *TerminalWindow::ldisc = NULL;
Mouse_Action TerminalWindow::lastact = MA_NOTHING;
Mouse_Button TerminalWindow::lastbtn = MBT_NOTHING;
int TerminalWindow::dbltime = 0, TerminalWindow::lasttime = 0;
int TerminalWindow::prev_rows = 0, TerminalWindow::prev_cols = 0;
int TerminalWindow::mouseX = 0;
int TerminalWindow::mouseY = 0;
long TerminalWindow::timing_next_time;
WPARAM TerminalWindow::pend_netevent_wParam = 0;
LPARAM TerminalWindow::pend_netevent_lParam = 0;
int TerminalWindow::pending_netevent = 0;
int TerminalWindow::kbd_codepage = 0;
struct sesslist TerminalWindow::sesslist;
HHOOK TerminalWindow::g_hKeyBoard;
HHOOK TerminalWindow::g_hKeyMouse;
HMENU TerminalWindow::savedsess_menu;
int TerminalWindow::show;
HINSTANCE TerminalWindow::prev;
HBITMAP TerminalWindow::caretbm;
//static HWND hwndNextViewer = NULL;
int first;
Properties *sesskey;
int TerminalWindow::is_alt_pressed(void) {
	BYTE keystate[256];
	int r = GetKeyboardState(keystate);
	if (!r)
		return FALSE;
	if (keystate[VK_MENU] & 0x80)
		return TRUE;
	if (keystate[VK_RMENU] & 0x80)
		return TRUE;
	return FALSE;
}

/*
 * Translate a raw mouse button designation (LEFT, MIDDLE, RIGHT)
 * into a cooked one (SELECT, EXTEND, PASTE).
 */
Mouse_Button TerminalWindow::translate_button(Mouse_Button button) {
	if (button == MBT_LEFT)
		return MBT_SELECT;
	if (button == MBT_MIDDLE)
		return cfg.mouse_is_xterm == 1 ? MBT_PASTE : MBT_EXTEND;
	if (button == MBT_RIGHT)
		return cfg.mouse_is_xterm == 1 ? MBT_EXTEND : MBT_PASTE;
	return MBT_LEFT; /* shouldn't happen */
}

void TerminalWindow::click(Mouse_Button b, int x, int y, int shift, int ctrl,
		int alt) {
	int thistime = GetMessageTime();

	if (send_raw_mouse && !(cfg.mouse_override && shift)) {
		lastbtn = MBT_NOTHING;
		term_mouse(term, b, translate_button(b), MA_CLICK, x, y, shift, ctrl,
				alt);
		return;
	}

	if (lastbtn == b && thistime - lasttime < dbltime) {
		lastact = (lastact == MA_CLICK ? MA_2CLK :
					lastact == MA_2CLK ? MA_3CLK :
					lastact == MA_3CLK ? MA_CLICK : MA_NOTHING);
	} else {
		lastbtn = b;
		lastact = MA_CLICK;
	}
	if (lastact != MA_NOTHING)
		term_mouse(term, b, translate_button(b), lastact, x, y, shift, ctrl,
				alt);
	lasttime = thistime;
}

TerminalWindow::TerminalWindow(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline,
		int show) {
	pending_netevent = 0;

}

TerminalWindow::~TerminalWindow() {
	// TODO Auto-generated destructor stub
}

void TerminalWindow::show_mouseptr(int show) {
	/* NB that the counter in ShowCursor() is also frobbed by
	 * update_mouse_pointer() */
	static int cursor_visible = 1;
	if (!cfg.hide_mouseptr) /* override if this feature disabled */
		show = 1;
	if (cursor_visible && !show)
		ShowCursor(FALSE);
	else if (!cursor_visible && show)
		ShowCursor(TRUE);
	cursor_visible = show;
}

LRESULT TerminalWindow::OnCreate(WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hwnd, WM_CREATE, wParam, lParam);
}

LRESULT TerminalWindow::OnDestroy(WPARAM wParam, LPARAM lParam) {
	show_mouseptr(1);
	PostQuitMessage(0);
	return DefWindowProc(hwnd, WM_DESTROY, wParam, lParam);
}

LRESULT TerminalWindow::OnClose(WPARAM wParam, LPARAM lParam) {
	char *str;
	show_mouseptr(1);
	str = dupprintf("%s", appname);
	if (!cfg.warn_on_close || session_closed
			|| MessageBox(hwnd, SURE_EXIT_MESSAGE, str,
					MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK) {
		DestroyWindow (hwnd);
	}
	sfree(str);
	return 0;
//	return DefWindowProc(hwnd, WM_CLOSE, wParam, lParam);
}

LRESULT TerminalWindow::OnTimer(WPARAM wParam, LPARAM lParam) {
	if ((UINT_PTR) wParam == TIMING_TIMER_ID) {
		long next;
		KillTimer(hwnd, TIMING_TIMER_ID);
		if (run_timers(timing_next_time, &next)) {
			timer_change_notify(next);
		}
	}
	return DefWindowProc(hwnd, WM_TIMER, wParam, lParam);
}

template<class T>
inline T WinGetLong(HWND hwnd, int which = GWL_USERDATA) {
	return reinterpret_cast<T>(::GetWindowLong(hwnd, which));
}

template<class T>
inline void WinSetLong(HWND hwnd, T value, int which = GWL_USERDATA) {
	::SetWindowLong(hwnd, which, reinterpret_cast<long>(value));
}

LRESULT CALLBACK TerminalWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam,
		LPARAM lParam) {

#ifdef DEBUG_CAN_INPUT
	int debug = 1; //=1时允许键盘输入，用于调试，正式版=0
#else
	int debug = 0; //=1时允许键盘输入，用于调试，正式版=0
#endif
	LRESULT ret = 0;
	::hwnd = hWnd;

	switch (message) {
	case WM_CREATE:
		ret = OnCreate(wParam, lParam);
		break;
	case WM_TIMER:
		ret = OnTimer(wParam, lParam);
		break;
	case WM_CLOSE:
		ret = OnClose(wParam, lParam);
		break;
	case WM_DESTROY:
		ret = OnDestroy(wParam, lParam);
		break;
	case WM_INITMENUPOPUP: //消息在下拉菜单或子菜单将要被激活的时候发出.
		ret = OnInitMenupopup(message, wParam, lParam);
		break;
	case WM_COMMAND: //接收菜单栏的消息
		ret = OnCommand(message, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		ret = OnButtonDownUp(message, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		/*
		 * Windows seems to like to occasionally send MOUSEMOVE
		 * events even if the mouse hasn't moved. Don't unhide
		 * the mouse pointer in this case.
		 */
		ret = OnMouseMove(message, wParam, lParam);
		break;
	case WM_NCMOUSEMOVE:
		ret = OnNcMouseMove(message, wParam, lParam);
		break;
	case WM_IGNORE_CLIP:
		ignore_clip = wParam; /* don't panic on DESTROYCLIPBOARD */
		break;
	case WM_DESTROYCLIPBOARD:
		if (!ignore_clip)
			term_deselect (term);
		ignore_clip = FALSE;
		ret = 0;
		break;
	case WM_PAINT:
		ret = OnPaint(message, wParam, lParam);
		break;
	case WM_NETEVENT:
		/* Notice we can get multiple netevents, FD_READ, FD_WRITE etc
		 * but the only one that's likely to try to overload us is FD_READ.
		 * This means buffering just one is fine.
		 */
		ret = OnNetEvent(message, wParam, lParam);
		break;
	case WM_SETFOCUS:
		ret = OnSetFocus(message, wParam, lParam);
		break;
	case WM_KILLFOCUS:
		ret = OnKillFocus(message, wParam, lParam);
		break;
	case WM_ENTERSIZEMOVE:
		EnableSizeTip(1);
		resizing = TRUE;
		need_backend_resize = FALSE;
		break;
	case WM_EXITSIZEMOVE:
		ret = OnExitSizeMove(message, wParam, lParam);
		break;
	case WM_SIZING:
		/*
		 * This does two jobs:
		 * 1) Keep the sizetip uptodate
		 * 2) Make sure the window size is _stepped_ in units of the font size.
		 */
		ret = OnSizing(message, wParam, lParam);
		break;
		/* break;  (never reached) */
	case WM_FULLSCR_ON_MAX:
		fullscr_on_max = TRUE;
		break;
	case WM_MOVE:
		sys_cursor_update();
		break;
	case WM_SIZE:
		ret = OnSize(message, wParam, lParam);
		break;
	case WM_VSCROLL:
		ret = OnVscroll(message, wParam, lParam);
		break;
	case WM_PALETTECHANGED:
		ret = OnPaletteChangerd(message, wParam, lParam);
		break;
	case WM_QUERYNEWPALETTE:
		ret = OnQueryNewpalette();
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYUP:
		if (debug || (enablekey == 1 && !comdev_reading)) {
			ret = OnSysKeyUp(message, wParam, lParam);
		} else {
			ret = DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;
	case WM_INPUTLANGCHANGE:
		ret = OnInputLangChange(message, wParam, lParam);
		break;
	case WM_IME_STARTCOMPOSITION:
		ret = OnImeStartComposition(message, wParam, lParam);
		break;
	case WM_IME_ENDCOMPOSITION:
		ret = OnImeEndComposition(message, wParam, lParam);
		break;
	case WM_IME_COMPOSITION:
		ret = OnImeComposition(message, wParam, lParam);
		break;
	case WM_IME_CHAR:
		if (debug || (enablekey == 1 && !comdev_reading)) {
			ret = OnImeChar(message, wParam, lParam);
		} else {
			ret = DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;
	case WM_CHAR:
	case WM_SYSCHAR:
		if (debug || (enablekey == 1 && !comdev_reading)) {
			ret = OnChar(message, wParam, lParam);
		} else {
			ret = DefWindowProc(hwnd, message, wParam, lParam);
		}
		break;
	case WM_SYSCOLORCHANGE:
		ret = OnSysColorChange(message, wParam, lParam);
		break;
	case WM_AGENT_CALLBACK:
		ret = OnAgentCallback(message, wParam, lParam);
		break;
	default:
		ret = DefWindowProc(hwnd, message, wParam, lParam);
		break;
	}

	return ret;
}

/*
 * Refresh the saved-session submenu from `sesslist'.
 */
void TerminalWindow::update_savedsess_menu(void) {
	int i;
	while (DeleteMenu(savedsess_menu, 0, MF_BYPOSITION))
		;
	/* skip sesslist.sessions[0] == Default Settings */
	for (i = 1;
			i
					< ((sesslist.nsessions <= MENU_SAVED_MAX + 1) ?
							sesslist.nsessions : MENU_SAVED_MAX + 1); i++)
		AppendMenu(savedsess_menu, MF_ENABLED,
				IDM_SAVED_MIN + (i - 1) * MENU_SAVED_STEP,
				sesslist.sessions[i]);
}

/*
 * Update the Special Commands submenu.
 */
void TerminalWindow::update_specials_menu(void *frontend) {
	HMENU new_menu;
	int i, j;

	if (back)
		specials = back->get_specials(backhandle);
	else
		specials = NULL;

	if (specials) {
		/* We can't use Windows to provide a stack for submenus, so
		 * here's a lame "stack" that will do for now. */
		HMENU saved_menu = NULL;
		int nesting = 1;
		new_menu = CreatePopupMenu();
		for (i = 0; nesting > 0; i++) {
			if (!(IDM_SPECIAL_MIN + 0x10 * i < IDM_SPECIAL_MAX)) {
				return;
			}
			//assert(IDM_SPECIAL_MIN + 0x10 * i < IDM_SPECIAL_MAX);
			switch (specials[i].code) {
			case TS_SEP:
				AppendMenu(new_menu, MF_SEPARATOR, 0, 0);
				break;
			case TS_SUBMENU:
				if (nesting >= 2) {
					return;
				}
				//assert(nesting < 2);
				nesting++;
				saved_menu = new_menu; /* XXX lame stacking */
				new_menu = CreatePopupMenu();
				AppendMenu(saved_menu, MF_POPUP | MF_ENABLED, (UINT) new_menu,
						specials[i].name);
				break;
			case TS_EXITMENU:
				nesting--;
				if (nesting) {
					new_menu = saved_menu; /* XXX lame stacking */
					saved_menu = NULL;
				}
				break;
			default:
				AppendMenu(new_menu, MF_ENABLED, IDM_SPECIAL_MIN + 0x10 * i,
						specials[i].name);
				break;
			}
		}
		/* Squirrel the highest special. */
		n_specials = i - 1;
	} else {
		new_menu = NULL;
		n_specials = 0;
	}

	for (j = 0; j < lenof(popup_menus); j++) {
		if (specials_menu) {
			/* XXX does this free up all submenus? */
			DeleteMenu(popup_menus[j].menu, (UINT) specials_menu, MF_BYCOMMAND);
			DeleteMenu(popup_menus[j].menu, IDM_SPECIALSEP, MF_BYCOMMAND);
		}
		if (new_menu) {
			InsertMenu(popup_menus[j].menu, IDM_SHOWLOG,
					MF_BYCOMMAND | MF_POPUP | MF_ENABLED, (UINT) new_menu,
					"&常用指令");
			InsertMenu(popup_menus[j].menu, IDM_SHOWLOG,
					MF_BYCOMMAND | MF_SEPARATOR, IDM_SPECIALSEP, 0);
		}
	}
	specials_menu = new_menu;
}

void TerminalWindow::update_mouse_pointer(void) {
	LPTSTR curstype;
	int force_visible = FALSE;
	static int forced_visible = FALSE;
	switch (busy_status) {
	case BUSY_NOT:
		if (send_raw_mouse)
			curstype = IDC_ARROW;
		else
			curstype = IDC_IBEAM;
		break;
	case BUSY_WAITING:
		curstype = IDC_APPSTARTING; /* this may be an abuse */
		force_visible = TRUE;
		break;
	case BUSY_CPU:
		curstype = IDC_WAIT;
		force_visible = TRUE;
		break;
	default:
		return;
	}
	{
		HCURSOR cursor = LoadCursor(NULL, curstype);
		SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG_PTR) cursor);
		SetCursor(cursor); /* force redraw of cursor at current posn */
	}
	if (force_visible != forced_visible) {
		/* We want some cursor shapes to be visible always.
		 * Along with show_mouseptr(), this manages the ShowCursor()
		 * counter such that if we switch back to a non-force_visible
		 * cursor, the previous visibility state is restored. */
		ShowCursor(force_visible);
		forced_visible = force_visible;
	}
}

LRESULT CALLBACK TerminalWindow::KeyBoardProc(int code, // hook code
		WPARAM wParam, // virtual-key code
		LPARAM lParam // keystroke-message information
		) {
	if (wParam == VK_SNAPSHOT) {
		term_print_screen (term);
		return 1;
	}
	return 0;
}

//鼠标钩子
LRESULT CALLBACK TerminalWindow::KeyMouseProc(int code, // hook code
		WPARAM wParam, // virtual-key code
		LPARAM lParam // keystroke-message information
		) {
	if (160 == wParam) {
		return 1;
	}
	return 0;
}

string GenerateMach(char * localRegistCod, int type) {
	char Serialnum[256];
	memset(Serialnum, 0x00, 256);
	int rv = 0;
	switch (type) {
	case 1:
		rv = getCpuSeleroNumber(Serialnum);
		break;
	case 2:
		rv = getHddSeleroNumber(Serialnum);
		break;
	case 3:
		rv = Util::getMacAddress(Serialnum);
		break;
	}
	int hash = Util::RSHash(Serialnum);
	char temp[4];
	memset(temp, 0x00, 4);
	sprintf(temp, "%04d", hash % 10000);
	strcat(localRegistCod, temp);
	return temp;
}

char* TerminalWindow::GetRegistCode() {
	static char localRegistKey[20];
	char localRegistCod[20];
	char registerkey[1024];
	int rv = 0;
	int hash = 0;
	memset(localRegistCod, 0x00, 20);
	//检查cpu序列号
	terminal::GenerateMach(localRegistCod, 1);
	localRegistCod[4] = '-';
	//检查硬盘序列号
	terminal::GenerateMach(localRegistCod, 2);
	localRegistCod[9] = '-';
	//检查机器码
	terminal::GenerateMach(localRegistCod, 3);

	//生成机器码:
	memset(registerkey, 0x00, 1024);

	//汇金
	rv = encrypt_3des("f<w$/_db", "slOx'4\\#", "*MiYzXQ#",
			(unsigned char *) localRegistCod, (unsigned char *) registerkey);

	hash = Util::RSHash(registerkey);
	sprintf(localRegistKey, "%06d", hash % 1000000);

	return localRegistKey;
}

bool TerminalWindow::RegistSuccess() {
	Properties prop;
	prop.SafeLoad(configpath);

	if (prop.GetString("RegistKey") == GetRegistCode()) {
		return true;
	} else {
	// 应该提示另一个注册程序
	//		ConfigDialog::ShowReg (hwnd);
		MessageBox(NULL, "nihao", "nihao", 0);
		return true;
	}

	return false;
}

void TerminalWindow::MonitorCom(char* title) {
	static char last = 0;
	string temp;
	if (comdev_char != last) {
		if (comdev_char) {
			temp = title;
			temp += CURRENTCOM;
			temp += comdev_char;
			set_title(NULL, (char *) temp.c_str());
		} else {
			set_title(NULL, title);
		}
		last = comdev_char;
	}
}

void TerminalWindow::set_cfgto_uchar() {
	for (int i = 1; i < KEYCODEFXX_MAX_NUM; i++) {
		KeyCodeLenFXX[i] = 0;
		for (unsigned int j = 0; j < strlen(cfg.keycodef[i]); j++) {
			if ((cfg.keycodef[i][j] == 0x5E)
					&& (cfg.keycodef[i][j + 1]) == 0x5B) {
				j++;
				KeyCodeFXX[i - 1][KeyCodeLenFXX[i - 1]++] = 0X1B;
			} else
				KeyCodeFXX[i - 1][KeyCodeLenFXX[i - 1]++] = cfg.keycodef[i][j];
		}

	}

}

void TerminalWindow::Run(HINSTANCE inst) { //, const char* name

	WNDCLASS wndclass; //定义窗口结构体类
	MSG msg; //消息结构体

	int nFullWidth = 0;
	int nFullHeight = 0;

	int fHeight = 0;
	int fWidth = 0;
	int guess_width, guess_height;

	::hinst = inst; //所有实体的句柄
	flags = FLAG_VERBOSE | FLAG_INTERACTIVE; //定义窗口类型

	if (!RegistSuccess()) {
		return;
	}
	set_cfgto_uchar();
	init_pairs();
	Update_KeyPairs();

	//加入钩子程序
	g_hKeyBoard = SetWindowsHookEx(WH_KEYBOARD, KeyBoardProc, NULL,
			GetCurrentThreadId());
//
//	g_hKeyMouse = SetWindowsHookEx(WH_MOUSE, KeyMouseProc, NULL,
//			GetCurrentThreadId());

	sk_init();
	init_winver();
	{
		nFullWidth = GetSystemMetrics(SM_CXSCREEN);
		nFullHeight = GetSystemMetrics(SM_CYSCREEN);
		if (nFullWidth == 1024 && nFullHeight == 768) {
			fHeight = 28;
			fWidth = 12;
		} else if (nFullWidth == 1280 && nFullHeight == 1024) {
			fHeight = 38;
			fWidth = 16;
		} else if (nFullWidth == 800 && nFullHeight == 600) {
			fHeight = 21;
			fWidth = 10;
		} else {
			fHeight = cfg.font.height;
			fWidth = cfg.fontwidth;
		}
	}
	//开始初始化新的窗口并打开新的窗口,先定义后注册
	if (!prev) { //初始化窗口类
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc; //为一指针，指向用户定义的该窗口的消息处理函数,处理窗口的各种消息
		wndclass.cbClsExtra = 0; //用于在窗口类结构中保留一定空间，用于存在自己需要的某些信息。
		wndclass.cbWndExtra = 0; //用于在Windows内部保存的窗口结构中保留一定空间。
		wndclass.hInstance = inst; //表示创建该窗口的程序的运行实体代号（WinMain的参数之一）。
		wndclass.hIcon = LoadIcon(inst, MAKEINTRESOURCE(IDI_MAINICON)); //hIcon、hCursor、hbrBackground、lpszMenuName分别表示该窗口的图标、鼠标形状、背景色以及菜单
		wndclass.hCursor = LoadIcon(inst, MAKEINTRESOURCE(IDI_MOUSE_CURSOR));
		wndclass.hbrBackground = NULL;
		wndclass.lpszMenuName = "GenericMenu";
		wndclass.lpszClassName = appname; //表示该窗口类别的名称，即标识该窗口类的标志。
		RegisterClass(&wndclass);
	}

	popup_menus[SYSMENU].menu = LoadMenu(inst, "GenericMenu");
	popup_menus[CTXMENU].menu = LoadMenu(inst, MAKEINTRESOURCE(IDR_MENU_POPUP));
	memset(&ucsdata, 0, sizeof(ucsdata)); //UNICODE 格式结构体

	cfgtopalette(); //复制颜色调色版
	//设置字体大小
	font_width = 10;
	font_height = 20;
	extra_width = 25;
	extra_height = 28;
	guess_width = extra_width + font_width * cfg.width;
	guess_height = extra_height + font_height * cfg.height;
	{
		RECT r;
		get_fullscreen_rect(&r); //取全屏幕的坐标
		if (guess_width > r.right - r.left)
			guess_width = r.right - r.left;
		if (guess_height > r.bottom - r.top)
			guess_height = r.bottom - r.top;
	}
	{
		int winmode = WS_OVERLAPPEDWINDOW | WS_VSCROLL;
		int exwinmode = 0;
		if (!cfg.scrollbar)
			winmode &= ~(WS_VSCROLL); //加入一个垂直滚动条
		if (cfg.resize_action == RESIZE_DISABLED)
			winmode &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX); //WS_THICKFRAME 可以拖动改变窗口大小,WS_MAXIMIZEBOX新窗口是否带有[最大化]按钮
		if (cfg.alwaysontop)
			exwinmode |= WS_EX_TOPMOST; //窗口设为顶层
		if (cfg.sunken_edge)
			exwinmode |= WS_EX_CLIENTEDGE; //Specifies that a window has a 3D look — that is, a border with a sunken edge.
		hwnd = CreateWindowEx(exwinmode, appname,
				appname, //exwinmode指定窗口的扩展风格,appname指向一个空结束的字符串或整型数,lpWindowName:指向一个指定窗口名的空结束的字符串指针。
				winmode, CW_USEDEFAULT, CW_USEDEFAULT, guess_width,
				guess_height, NULL, NULL, inst, NULL); //该函数创建一个具有扩展风格的重叠式窗口、弹出式窗口或子窗口，其他与 CreateWindow函数相同。
	}

	//初始化仿真终端模块
	term = term_init(&cfg, &ucsdata, NULL); //初始化终端
	logctx = log_init(NULL, &cfg);
	term_provide_logctx(term, logctx); //把配置信息复制给term对象终端对象
	term_size(term, cfg.height, cfg.width, cfg.savelines); //设置终端的大小

	//初始化显示字体
	init_fonts(fWidth, fHeight);
//	init_fonts(nFullWidth / cfg.width, (nFullHeight - 20) / cfg.height);
	//设置屏幕坐标
	{
		RECT cr, wr;
		GetWindowRect(hwnd, &wr);
		GetClientRect(hwnd, &cr);
		offset_width = offset_height = cfg.window_border;
		extra_width = wr.right - wr.left - cr.right + cr.left
				+ offset_width * 2;
		extra_height = wr.bottom - wr.top - cr.bottom + cr.top
				+ offset_height * 2;
	}

	/*
	 * Resize the window, now we know what size we _really_ want it
	 * to be.
	 */
	guess_width = extra_width + font_width * term->cols;
	guess_height = extra_height + font_height * term->rows;
	SetWindowPos(hwnd, NULL, 0, 0, guess_width, guess_height,
			SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER); //设置窗口大小

	/*
	 * Initialise the scroll bar.
	 */
	{
		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
		si.nMin = 0;
		si.nMax = term->rows - 1;
		si.nPage = term->rows;
		si.nPos = 0;
		SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
	}

	/*
	 * Prepare the mouse handler.
	 *准备鼠标句柄
	 */
	lastact = MA_NOTHING;
	lastbtn = MBT_NOTHING;
	dbltime = GetDoubleClickTime(); //该函数取得鼠标的当前双击时间

	start_backend();

	/*
	 * Set up the initial input locale.
	 */
	set_input_locale(GetKeyboardLayout(0)); //获取本地化的参数，因系统不同国家不同

	/*
	 * Finally show the window!
	 */
	ShowWindow(hwnd, show);
	SetForegroundWindow (hwnd);

	/*
	 * Set the palette up.
	 */
	pal = NULL; //句柄
	logpal = NULL; //逻辑调色板结构体
	init_palette(); //初始化调色板

	PostMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0); //窗口一打开就最大化
	term_set_focus(term, GetForegroundWindow() == hwnd); //设置焦点
	UpdateWindow(hwnd);
	enablekey = 1;
	//进入消息循环,等待消息
	static char titlename[100];
	GetWindowTextA(hwnd, titlename, 100);
	while (1) {
		HANDLE *handles;
		int nhandles, n;
		MonitorCom(titlename);
		handles = handle_get_events(&nhandles);

		n = MsgWaitForMultipleObjects(nhandles, handles, FALSE, INFINITE,
				QS_ALLINPUT); //类似WaitForMultipleObjects()，但它会在“对象被激发”或“消息到达队列”时被唤醒而返回

		if ((unsigned) (n - WAIT_OBJECT_0) < (unsigned) nhandles) {
			handle_got_event(handles[n - WAIT_OBJECT_0]);
			sfree(handles);
			if (must_close_session) {
				close_session();
				PostMessageA(hwnd, WM_COMMAND, IDM_RESTARTTELNET, 0);
				must_close_session = 0;
			}

		} else
			sfree(handles);

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

			if (msg.message == WM_QUIT) {
				goto finished;
			}
			/* two-level break */ //去掉后，则不会主动退出，只能手动退出
			if (!(IsWindow(logbox) && IsDialogMessage(logbox, &msg)))
				DispatchMessage(&msg); //该函数分发一个消息给窗口程序
			/* Send the paste buffer if there's anything to send */
			term_paste (term);
			/* If there's nothing new in the queue then we can do everything
			 * we've delayed, reading the socket, writing, and repainting
			 * the window.
			 */
			if (must_close_session) {
				close_session();
				PostMessageA(hwnd, WM_COMMAND, IDM_RESTARTTELNET, 0);
				must_close_session = 0;
			}
		}
		term_set_focus(term, GetForegroundWindow() == hwnd);

		if (pending_netevent)
			enact_pending_netevent();
		net_pending_errors();
	}

	finished: log_write_str(SYS_INFO, "EXIT");
	cleanup_exit(msg.wParam); /* this doesn't return... */
//	return msg.wParam; /* ... but optimiser doesn't know */
}

void TerminalWindow::close_session(void) {
	char morestuff[100];
	unsigned int i;
	session_closed = TRUE;
	sprintf(morestuff, "%.70s (%s)", appname, INACTIVE);
	set_icon(NULL, morestuff);
	set_title(NULL, morestuff);

	if (ldisc) {
		ldisc_free(ldisc);
		ldisc = NULL;
	}
	if (back) {
		back->free(backhandle);
		backhandle = NULL;
		back = NULL;
		term_provide_resize_fn(term, NULL, NULL);
		update_specials_menu(NULL);
	}
	/*
	 * Show the Restart Session menu item. Do a precautionary
	 * delete first to ensure we never end up with more than one.
	 */
	for (i = 0; i < lenof(popup_menus); i++) {
		DeleteMenu(popup_menus[i].menu, IDM_RESTART, MF_BYCOMMAND);
		InsertMenu(popup_menus[i].menu, IDM_DUPSESS, MF_BYCOMMAND | MF_ENABLED,
				IDM_RESTART, "&Restart Session");
	}
}

void TerminalWindow::enact_pending_netevent(void) {
	static int reentering = 0;
//	extern int select_result(WPARAM, LPARAM);

	if (reentering)
		return; /* don't unpend the pending */

	pending_netevent = FALSE;

	reentering = 1;
	select_result(pend_netevent_wParam, pend_netevent_lParam);
	reentering = 0;
}

void TerminalWindow::cfgtopalette(void) {
	int i;
	static const int ww[] = { 256, 257, 258, 259, 260, 261, 0, 8, 1, 9, 2, 10,
			3, 11, 4, 12, 5, 13, 6, 14, 7, 15 };

	for (i = 0; i < 22; i++) {
		int w = ww[i];
		defpal[w].rgbtRed = cfg.colours[i][0];
		defpal[w].rgbtGreen = cfg.colours[i][1];
		defpal[w].rgbtBlue = cfg.colours[i][2];
	}
	for (i = 0; i < NEXTCOLOURS; i++) {
		if (i < 216) {
			int r = i / 36, g = (i / 6) % 6, b = i % 6;
			defpal[i + 16].rgbtRed = r ? r * 40 + 55 : 0;
			defpal[i + 16].rgbtGreen = g ? g * 40 + 55 : 0;
			defpal[i + 16].rgbtBlue = b ? b * 40 + 55 : 0;
		} else {
			int shade = i - 216;
			shade = shade * 10 + 8;
			defpal[i + 16].rgbtRed = defpal[i + 16].rgbtGreen =
					defpal[i + 16].rgbtBlue = shade;
		}
	}

	/* Override with system colours if appropriate */
	if (cfg.system_colour)
		systopalette();
}

void TerminalWindow::start_backend(void) {
	const char *error;
	char msg[1024], *title;
	char *realhost;
	int i;

	/*
	 * Select protocol. This is farmed out into a table in a
	 * separate file to enable an ssh-free variant.
	 */
	back = NULL;
	for (i = 0; backends[i].backend != NULL; i++)
		if (backends[i].protocol == cfg.protocol) {
			back = backends[i].backend;
			break;
		}

	if (back == NULL) {
		char *str = dupprintf("%s Internal Error", appname);
		MessageBox(NULL, "Unsupported protocol number found", str,
				MB_OK | MB_ICONEXCLAMATION);
		sfree(str);
		cleanup_exit(1);
	}

	error = back->init(NULL, &backhandle, &cfg, cfg.host, cfg.port, &realhost,
			cfg.tcp_nodelay, cfg.tcp_keepalives);
	back->provide_logctx(backhandle, logctx);
	if (error) {
		char *str = dupprintf("%s Error", appname);
		sprintf(msg, ERROR_NOTCONNECT
		"%.800s\n" "%s", cfg_dest(&cfg), error);
		MessageBox(NULL, msg, str, MB_ICONERROR | MB_OK);
		sfree(str);
		BeforClose (term);
		exit(0);
	}
	window_name = icon_name = NULL;
	if (*cfg.wintitle) {
		title = cfg.wintitle;
	} else {
		sprintf(msg, "%s - %s   %s:%s    section:%s  %s:%s ", realhost,
				appname, CURRENTUSER, sysusername, sectionName, SCREENNUM,
				cfg.screennum);
		title = msg;
	}
	sfree(realhost);
	set_title(NULL, title);
	set_icon(NULL, title);

	/*
	 * Connect the terminal to the backend for resize purposes.
	 */
	term_provide_resize_fn(term, back->size, backhandle);

	/*
	 * Set up a line discipline.
	 */
	ldisc = ldisc_create(&cfg, term, back, backhandle, NULL);

	/*
	 * Destroy the Restart Session menu item. (This will return
	 * failure if it's already absent, as it will be the very first
	 * time we call this function. We ignore that, because as long
	 * as the menu item ends up not being there, we don't care
	 * whether it was us who removed it or not!)
	 */
	for (i = 0; i < lenof(popup_menus); i++) {
		DeleteMenu(popup_menus[i].menu, IDM_RESTART, MF_BYCOMMAND);
	}

	must_close_session = FALSE;
	session_closed = FALSE;
}

void TerminalWindow::set_input_locale(HKL kl) {
	char lbuf[20];

	GetLocaleInfo(LOWORD(kl), LOCALE_IDEFAULTANSICODEPAGE, lbuf, sizeof(lbuf)); //可以获取各种数据的设置参数。配合GetDateFormat,GetTimeFormat可以获的日期、时间的本地格式化结果

	kbd_codepage = atoi(lbuf);
}

LRESULT TerminalWindow::OnCommand(UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId, wmEvent;
	wmId = LOWORD(wParam);
	wmEvent = HIWORD(wParam);
	switch (wmId) {
	case IDM_COPY_PASTE:
		term_do_copy (term);
		DefWindowProc(hwnd, WM_COMMAND, wParam, lParam);
		if (enablekey == 1 && !comdev_reading) {
			if (OpenClipboard (hwnd)) {
				HANDLE hData = GetClipboardData(CF_TEXT);
				buffer = (char*) GlobalLock(hData);
				if (buffer == NULL) {
					return -1;
				}
				GlobalUnlock(hData);
				CloseClipboard();
				if (!comdev_reading && (ldisc)) {
					lpage_send(ldisc, kbd_codepage, buffer, strlen(buffer), 1);
				}
				return 0;
			}
		}
		return -1;
		break;
	case IDM_PASTE:
		if (enablekey == 1 && !comdev_reading) {
			if (OpenClipboard (hwnd)) {
				HANDLE hData = GetClipboardData(CF_TEXT);
				buffer = (char*) GlobalLock(hData);
				if (buffer == NULL) {
					return -1;
				}
				GlobalUnlock(hData);
				CloseClipboard();
				if (!comdev_reading && (ldisc)) {
					lpage_send(ldisc, kbd_codepage, buffer, strlen(buffer), 1);
				}
				return 0;
			}
		}
		return -1;
		break;
	case IDM_COPY:
		term_do_copy (term);
		break;
	case IDM_RESTARTTELNET:
		if (!back) {
			start_backend();
			term_pwron(term, FALSE);
		}
		break;
	case IDM_INACTIVE:
		close_session();
		break;
	case IDM_NEWTELNET: {
//		ConfigDialog::ShowNewTerm (hwnd);
	}
		break;
	case IDM_ALLSCREENSHOW:
		if ((ishaveMenu == 1) && (myIsFullSC == 0)) //如果原来是1说明有菜单,去掉菜单
				{
			SetMenu(hwnd, NULL); //去掉菜单栏
			isShowMenu = 0;
			myIsFullSC = 1;
		}
		flip_full_screen();
		break;
	case IDM_ZXH:
		ShowWindow(hwnd, SW_MINIMIZE);
		break;
	case IDM_HELPDOC:
//		ConfigDialog::ShowHelp (hwnd);
		break;
	case IDM_DESCRIPTION:
		char pdfexe[MAX_PATH];
		memset(pdfexe, 0x00, MAX_PATH);
		sprintf(pdfexe, "%s\\pdfreader.exe", currentpath);
		char pdfpath[MAX_PATH];
		memset(pdfpath, 0x00, MAX_PATH);
		sprintf(pdfpath, "\"%s\\readme.pdf\"", currentpath);
		Util::ShellExecuteNoWait(pdfexe, pdfpath);
		break;
	case IDM_CAL: {
		WinExec("calc.exe", SW_SHOW);
	}
		break;
	case IDM_EXIT_SYSTEM:
	case IDM_EXIT: {
		char *str;
		show_mouseptr(1);
		str = dupprintf("%s", appname);
		if (!cfg.warn_on_close || session_closed
				|| MessageBox(hwnd, SURE_CLOSE_CURRENTWIND, str,
						MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON1) == IDOK) {
			DestroyWindow(hwnd);
		}
		sfree(str);
	}
		break;
	case IDM_PRINT_SCREEN: {
		term_print_screen(term);
	}
		break;
	case IDM_COPY_SCREEN: {
		term_copy_screen(term);
	}
		break;
	case IDM_TEST: {
//		if (strlen(cfg.adminpassword) != 0) {
//			ConfigDialog::ShowAdmin(hwnd);
//			if (cfg.admin == 1) {
//				ConfigDialog::ShowSet(hwnd);
//			}
//		} else {
//			ConfigDialog::ShowSet(hwnd);
//		}
	}
		break;
	}
	return DefWindowProc(hwnd, WM_COMMAND, wParam, lParam);
}

LRESULT TerminalWindow::OnButtonDownUp(UINT message, WPARAM wParam,
		LPARAM lParam) {
	if (message == WM_RBUTTONDOWN
			&& ((wParam & MK_CONTROL) || (cfg.mouse_is_xterm == 2))) {
		POINT cursorpos;

		show_mouseptr(1); /* make sure pointer is visible */
		GetCursorPos(&cursorpos);
		TrackPopupMenu(GetSubMenu(popup_menus[CTXMENU].menu, 1),
				TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, cursorpos.x,
				cursorpos.y, 0, hwnd, NULL);
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	{
		Mouse_Button button;
		int press;

		switch (message) {
		case WM_LBUTTONDOWN:
			button = MBT_LEFT;
			press = 1;
			break;
		case WM_MBUTTONDOWN:
			button = MBT_MIDDLE;
			press = 1;
			break;
		case WM_RBUTTONDOWN: {
			button = MBT_RIGHT;
			press = 0;
			break;
		}
		case WM_LBUTTONUP:
			button = MBT_LEFT;
			press = 0;
			break;
		case WM_MBUTTONUP:
			button = MBT_MIDDLE;
			press = 0;
			break;
		case WM_RBUTTONUP: {
			button = MBT_RIGHT;
			press = 0;
			POINT cursorpos;
			GetCursorPos(&cursorpos);
			TrackPopupMenu(GetSubMenu(popup_menus[CTXMENU].menu, 0),
					TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, cursorpos.x,
					cursorpos.y, 0, hwnd, NULL);
			break;
		}
		default:
			press = 0;
			break;
		}
		show_mouseptr(1);
		/*
		 * Special case: in full-screen mode, if the left
		 * button is clicked in the very top left corner of the
		 * window, we put up the System menu instead of doing
		 * selection.
		 */
		{
			char mouse_on_hotspot = 0;
			POINT pt;

			GetCursorPos(&pt);
#ifndef NO_MULTIMON
			{
				HMONITOR mon;
				MONITORINFO mi;

				mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);

				if (mon != NULL) {
					mi.cbSize = sizeof(MONITORINFO);
					GetMonitorInfo(mon, &mi);

					if (mi.rcMonitor.left == pt.x && mi.rcMonitor.top == pt.y) {
						mouse_on_hotspot = 1;
					}
				}
			}
#else
			if (pt.x == 0 && pt.y == 0) {
				mouse_on_hotspot = 1;
			}
#endif
			if (is_full_screen() && press && button == MBT_LEFT
					&& mouse_on_hotspot) {
				SendMessage(hwnd, WM_SYSCOMMAND, SC_MOUSEMENU,
						MAKELPARAM(pt.x, pt.y));
				return 0;
			}
		}
#define X_POS(l) ((int)(short)LOWORD(l))
#define Y_POS(l) ((int)(short)HIWORD(l))

#define TO_CHR_X(x) ((((x)<0 ? (x)-font_width+1 : (x))-offset_width) / font_width)
#define TO_CHR_Y(y) ((((y)<0 ? (y)-font_height+1: (y))-offset_height) / font_height)
		if (press) {
			click(button, TO_CHR_X(X_POS(lParam)), TO_CHR_Y(Y_POS(lParam)),
					wParam & MK_SHIFT, wParam & MK_CONTROL, is_alt_pressed());
			SetCapture (hwnd);
		} else {
			term_mouse(term, button, translate_button(button), MA_RELEASE,
					TO_CHR_X(X_POS(lParam)), TO_CHR_Y(Y_POS(lParam)),
					wParam & MK_SHIFT, wParam & MK_CONTROL, is_alt_pressed());
			ReleaseCapture();
		}
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnSetFocus(UINT message, WPARAM wParam, LPARAM lParam) {
	term_set_focus(term, TRUE);
	flash_window(0); /* stop */
	compose_state = 0;
	term_update (term);
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnMouseMove(UINT message, WPARAM wParam,
		LPARAM lParam) {
#define X_POS(l) ((int)(short)LOWORD(l))
#define Y_POS(l) ((int)(short)HIWORD(l))

#define TO_CHR_X(x) ((((x)<0 ? (x)-font_width+1 : (x))-offset_width) / font_width)
#define TO_CHR_Y(y) ((((y)<0 ? (y)-font_height+1: (y))-offset_height) / font_height)
	static WPARAM wp = 0;
	static LPARAM lp = 0;
	if (wParam != wp || lParam != lp || last_mousemove != WM_MOUSEMOVE) {
		show_mouseptr(1);
		wp = wParam;
		lp = lParam;
		//不断获取鼠标位置
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);
		last_mousemove = WM_MOUSEMOVE;
	}
	term_select_copy(term, TO_CHR_X(X_POS(lParam)), TO_CHR_Y(Y_POS(lParam)));

	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnNcMouseMove(UINT message, WPARAM wParam,
		LPARAM lParam) {
	static WPARAM wp = 0;
	static LPARAM lp = 0;
//	busy_status = BUSY_NOT;
//	update_mouse_pointer();

	if (wParam != wp || lParam != lp || last_mousemove != WM_NCMOUSEMOVE) {
		show_mouseptr(1);
		wp = wParam;
		lp = lParam;
		last_mousemove = WM_NCMOUSEMOVE;
	}

	noise_ultralight(lParam);
	return DefWindowProc(hwnd, message, wParam, lParam);

}
LRESULT TerminalWindow::OnPaint(UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT p;
	HideCaret (hwnd);
	HDC hdc = BeginPaint(hwnd, &p);
	if (pal) {
		SelectPalette(hdc, pal, TRUE);
		RealizePalette(hdc);
	}

	/*
	 * We have to be careful about term_paint(). It will
	 * set a bunch of character cells to INVALID and then
	 * call do_paint(), which will redraw those cells and
	 * _then mark them as done_. This may not be accurate:
	 * when painting in WM_PAINT context we are restricted
	 * to the rectangle which has just been exposed - so if
	 * that only covers _part_ of a character cell and the
	 * rest of it was already visible, that remainder will
	 * not be redrawn at all. Accordingly, we must not
	 * paint any character cell in a WM_PAINT context which
	 * already has a pending update due to terminal output.
	 * The simplest solution to this - and many, many
	 * thanks to Hung-Te Lin for working all this out - is
	 * not to do any actual painting at _all_ if there's a
	 * pending terminal update: just mark the relevant
	 * character cells as INVALID and wait for the
	 * scheduled full update to sort it out.
	 *
	 * I have a suspicion this isn't the _right_ solution.
	 * An alternative approach would be to have terminal.c
	 * separately track what _should_ be on the terminal
	 * screen and what _is_ on the terminal screen, and
	 * have two completely different types of redraw (one
	 * for full updates, which syncs the former with the
	 * terminal itself, and one for WM_PAINT which syncs
	 * the latter with the former); yet another possibility
	 * would be to have the Windows front end do what the
	 * GTK one already does, and maintain a bitmap of the
	 * current terminal appearance so that WM_PAINT becomes
	 * completely trivial. However, this should do for now.
	 */
	term_paint(term, hdc, (p.rcPaint.left - offset_width) / font_width,
			(p.rcPaint.top - offset_height) / font_height,
			(p.rcPaint.right - offset_width - 1) / font_width,
			(p.rcPaint.bottom - offset_height - 1) / font_height,
			!term->window_update_pending);

	if (p.fErase || p.rcPaint.left < offset_width
			|| p.rcPaint.top < offset_height
			|| p.rcPaint.right >= offset_width + font_width * term->cols
			|| p.rcPaint.bottom >= offset_height + font_height * term->rows) {
		HBRUSH fillcolour, oldbrush;
		HPEN edge, oldpen;
		fillcolour = CreateSolidBrush(colours[ATTR_DEFBG >> ATTR_BGSHIFT]);
		oldbrush = (HBRUSH) SelectObject(hdc, fillcolour);
		edge = CreatePen(PS_SOLID, 0, colours[ATTR_DEFBG >> ATTR_BGSHIFT]);
		oldpen = (HPEN) SelectObject(hdc, (HPEN) edge);

		/*
		 * Jordan Russell reports that this apparently
		 * ineffectual IntersectClipRect() call masks a
		 * Windows NT/2K bug causing strange display
		 * problems when the PuTTY window is taller than
		 * the primary monitor. It seems harmless enough...
		 */
		IntersectClipRect(hdc, p.rcPaint.left, p.rcPaint.top, p.rcPaint.right,
				p.rcPaint.bottom);

		ExcludeClipRect(hdc, offset_width, offset_height,
				offset_width + font_width * term->cols,
				offset_height + font_height * term->rows);

		Rectangle(hdc, p.rcPaint.left, p.rcPaint.top, p.rcPaint.right,
				p.rcPaint.bottom);

		/* SelectClipRgn(hdc, NULL); */

		SelectObject(hdc, oldbrush);
		DeleteObject(fillcolour);
		SelectObject(hdc, oldpen);
		DeleteObject(edge);
	}
	SelectObject(hdc, GetStockObject(SYSTEM_FONT));
	SelectObject(hdc, GetStockObject(WHITE_PEN));
	EndPaint(hwnd, &p);
	ShowCaret(hwnd);

	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnNetEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if (pending_netevent)
		enact_pending_netevent();

	pending_netevent = TRUE;
	pend_netevent_wParam = wParam;
	pend_netevent_lParam = lParam;
	if (WSAGETSELECTEVENT(lParam) != FD_READ) {
		enact_pending_netevent();
	}
	net_pending_errors();
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnKillFocus(UINT message, WPARAM wParam,
		LPARAM lParam) {
	show_mouseptr(1);
	term_set_focus(term, FALSE);
	DestroyCaret();
	caret_x = caret_y = -1; /* ensure caret is replaced next time */
	term_update (term);
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnInitMenupopup(UINT message, WPARAM wParam,
		LPARAM lParam) {
	if ((HMENU) wParam == savedsess_menu) {
		/* About to pop up Saved Sessions sub-menu.
		 * Refresh the session list. */
//		get_sesslist(&sesslist, FALSE); /* free */
//		get_sesslist(&sesslist, TRUE);
		update_savedsess_menu();
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnExitSizeMove(UINT message, WPARAM wParam,
		LPARAM lParam) {
	EnableSizeTip(0);
	resizing = FALSE;
#ifdef RDB_DEBUG_PATCH
	debug((27, "WM_EXITSIZEMOVE"));
#endif
	if (need_backend_resize) {
		term_size(term, cfg.height, cfg.width, cfg.savelines);
		InvalidateRect(hwnd, NULL, TRUE);
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnSizing(UINT message, WPARAM wParam, LPARAM lParam) {
	if (cfg.resize_action != RESIZE_FONT && !is_alt_pressed()) {
		int width, height, w, h, ew, eh;
		LPRECT r = (LPRECT) lParam;

		if (!need_backend_resize && cfg.resize_action == RESIZE_EITHER
				&& (cfg.height != term->rows || cfg.width != term->cols)) {
			/*
			 * Great! It seems that both the terminal size and the
			 * font size have been changed and the user is now dragging.
			 *
			 * It will now be difficult to get back to the configured
			 * font size!
			 *
			 * This would be easier but it seems to be too confusing.

			 term_size(term, cfg.height, cfg.width, cfg.savelines);
			 reset_window(2);
			 */
			cfg.height = term->rows;
			cfg.width = term->cols;

			InvalidateRect(hwnd, NULL, TRUE);
			need_backend_resize = TRUE;
		}

		width = r->right - r->left - extra_width;
		height = r->bottom - r->top - extra_height;
		w = (width + font_width / 2) / font_width;
		if (w < 1)
			w = 1;
		h = (height + font_height / 2) / font_height;
		if (h < 1)
			h = 1;
		UpdateSizeTip(hwnd, w, h);
		ew = width - w * font_width;
		eh = height - h * font_height;
		if (ew != 0) {
			if (wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT
					|| wParam == WMSZ_TOPLEFT)
				r->left += ew;
			else
				r->right -= ew;
		}
		if (eh != 0) {
			if (wParam == WMSZ_TOP || wParam == WMSZ_TOPRIGHT
					|| wParam == WMSZ_TOPLEFT)
				r->top += eh;
			else
				r->bottom -= eh;
		}
		if (ew || eh)
			return 1;
		else
			return 0;
	} else {
		int width, height, w, h, rv = 0;
		int ex_width = extra_width + (cfg.window_border - offset_width) * 2;
		int ex_height = extra_height + (cfg.window_border - offset_height) * 2;
		LPRECT r = (LPRECT) lParam;

		width = r->right - r->left - ex_width;
		height = r->bottom - r->top - ex_height;

		w = (width + term->cols / 2) / term->cols;
		h = (height + term->rows / 2) / term->rows;
		if (r->right != r->left + w * term->cols + ex_width)
			rv = 1;

		if (wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT
				|| wParam == WMSZ_TOPLEFT)
			r->left = r->right - w * term->cols - ex_width;
		else
			r->right = r->left + w * term->cols + ex_width;

		if (r->bottom != r->top + h * term->rows + ex_height)
			rv = 1;

		if (wParam == WMSZ_TOP || wParam == WMSZ_TOPRIGHT
				|| wParam == WMSZ_TOPLEFT)
			r->top = r->bottom - h * term->rows - ex_height;
		else
			r->bottom = r->top + h * term->rows + ex_height;

		return rv;
	}
}

LRESULT TerminalWindow::OnSize(UINT message, WPARAM wParam, LPARAM lParam) {
#ifdef RDB_DEBUG_PATCH
	debug((27, "WM_SIZE %s (%d,%ld)",
					(wParam == SIZE_MINIMIZED) ? "SIZE_MINIMIZED":
					(wParam == SIZE_MAXIMIZED) ? "SIZE_MAXIMIZED":
					(wParam == SIZE_RESTORED && resizing) ? "to":
					(wParam == SIZE_RESTORED) ? "SIZE_RESTORED":
					"...",
					LOWORD(lParam), HIWORD(lParam)));
#endif
	if (wParam == SIZE_MINIMIZED) {
		SetWindowText(hwnd, cfg.win_name_always ? window_name : icon_name);
	} else if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED) {
		SetWindowText(hwnd, window_name);
	}

	if (wParam == SIZE_RESTORED) {
		if ((ishaveMenu == 1) && (myIsFullSC == 1)) //如果原来是1说明原来有菜单，则重新显示菜单
				{
			HMENU menu;
			menu = LoadMenu(hinst, "GenericMenu"); //显示菜单 也可改变菜单项
			SetMenu(hwnd, menu);
			isShowMenu = 1;
			ishaveMenu = 1;
			myIsFullSC = 0;
		}
		if ((fullscr_on_max == TRUE) && (myIsFullSC == 1)) {
			SetMenu(hwnd, NULL); //去掉菜单栏
			isShowMenu = 0;
		}
		clear_full_screen();
	}
	if (wParam == SIZE_MAXIMIZED && fullscr_on_max) {
		fullscr_on_max = FALSE;
		if (ishaveMenu == 1) //如果原来是1说明有菜单,去掉菜单
				{
			SetMenu(hwnd, NULL); //去掉菜单栏
			isShowMenu = 0;
		}
		myIsFullSC = 1;

		make_full_screen();

	}

	if (cfg.resize_action == RESIZE_DISABLED) {
		/* A resize, well it better be a minimize. */
		reset_window(-1);
	} else {

		int width, height, w, h;

		width = LOWORD(lParam);
		height = HIWORD(lParam);

		if (!resizing) {
			if (wParam == SIZE_MAXIMIZED && !was_zoomed) {
				was_zoomed = 1;
				prev_rows = term->rows;
				prev_cols = term->cols;
				if (cfg.resize_action == RESIZE_TERM) {
					w = width / font_width;
					if (w < 1)
						w = 1;
					h = height / font_height;
					if (h < 1)
						h = 1;

					term_size(term, h, w, cfg.savelines);
				}
				reset_window(0);
			} else if (wParam == SIZE_RESTORED && was_zoomed) {
				was_zoomed = 0;
				if (cfg.resize_action == RESIZE_TERM)
					term_size(term, prev_rows, prev_cols, cfg.savelines);
				if (cfg.resize_action != RESIZE_FONT)
					reset_window(2);
				else
					reset_window(0);
			}
			/* This is an unexpected resize, these will normally happen
			 * if the window is too large. Probably either the user
			 * selected a huge font or the screen size has changed.
			 *
			 * This is also called with minimize.
			 */
			else
				reset_window(-1);
		}

		/*
		 * Don't call back->size in mid-resize. (To prevent
		 * massive numbers of resize events getting sent
		 * down the connection during an NT opaque drag.)
		 */
		if (resizing) {
			if (cfg.resize_action != RESIZE_FONT && !is_alt_pressed()) {
				need_backend_resize = TRUE;
				w = (width - cfg.window_border * 2) / font_width;
				if (w < 1)
					w = 1;
				h = (height - cfg.window_border * 2) / font_height;
				if (h < 1)
					h = 1;

				cfg.height = h;
				cfg.width = w;
			} else
				reset_window(0);
		}
	}
	sys_cursor_update();
	return 0;
}

LRESULT TerminalWindow::OnVscroll(UINT message, WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {
	case SB_BOTTOM:
		term_scroll(term, -1, 0);
		break;
	case SB_TOP:
		term_scroll(term, +1, 0);
		break;
	case SB_LINEDOWN:
		term_scroll(term, 0, +1);
		break;
	case SB_LINEUP:
		term_scroll(term, 0, -1);
		break;
	case SB_PAGEDOWN:
		term_scroll(term, 0, +term->rows / 2);
		break;
	case SB_PAGEUP:
		term_scroll(term, 0, -term->rows / 2);
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		term_scroll(term, 1, HIWORD(wParam));
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnPaletteChangerd(UINT message, WPARAM wParam,
		LPARAM lParam) {
	if ((HWND) wParam != hwnd && pal != NULL) {
		HDC hdc = get_ctx(NULL);
		if (hdc) {
			if (RealizePalette(hdc) > 0)
				UpdateColors(hdc);
			free_ctx(hdc);
		}
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL TerminalWindow::OnQueryNewpalette() {
	if (pal != NULL) {
		HDC hdc = get_ctx(NULL);
		if (hdc) {
			if (RealizePalette(hdc) > 0)
				UpdateColors(hdc);
			free_ctx(hdc);
			return TRUE;
		}
	}
	return FALSE;
}

//zhangbo
LRESULT TerminalWindow::OnKeyDown(UINT message, WPARAM wParam, LPARAM lParam) {
//	if (wParam == 192) {
//		if ((ishaveMenu == 1) && (myIsFullSC == 0)) //如果原来是1说明有菜单,去掉菜单
//				{
//			//SetMenu(hwnd,NULL); //去掉菜单栏
//			//isShowMenu=0;
//			//flip_full_screen();
//			//myIsFullSC=1;
//			//return TRUE;
//		} else {
//			flip_full_screen();
//			myIsFullSC = 0;
//			return TRUE;
//		}
//	}
	return OnSysKeyUp(message, wParam, lParam);
}

LRESULT TerminalWindow::OnSysKeyUp(UINT message, WPARAM wParam, LPARAM lParam) {

	/*
	 * Add the scan code and keypress timing to the random
	 * number noise.
	 */
	noise_ultralight(lParam);

	/*
	 * We don't do TranslateMessage since it disassociates the
	 * resulting CHAR message from the KEYDOWN that sparked it,
	 * which we occasionally don't want. Instead, we process
	 * KEYDOWN, and call the Win32 translator functions so that
	 * we get the translations under _our_ control.
	 */
	{
		unsigned char buf[20];
		int len = 0;

		if (message == WM_SYSKEYUP && wParam == 13 && (lParam & (1 << 29))) {
			flip_full_screen();
			return 0;
		}

		if (wParam == VK_PROCESSKEY) { /* IME PROCESS key */
			if (message == WM_KEYDOWN) {
				MSG m;
				m.hwnd = hwnd;
				m.message = WM_KEYDOWN;
				m.wParam = wParam;
				m.lParam = lParam & 0xdfff;
				TranslateMessage(&m);
			} else {
				return DefWindowProc(hwnd, message, wParam, lParam);
			}
		} else {
			memset(buf, 0x00, 20);

			static char strBuf[32];
			static int strLen;
			static boolean isPressDh;

			if (wParam == VK_F12) {
				isPressDh = !isPressDh;
				if (!isPressDh) { //esc
					memset(strBuf, 0x00, 32);
					strLen = 0;
					isPressDh = 0;
				}
				return 0;
			}

			if (isPressDh == 1) {
				if (wParam >= '0' && wParam <= '9') {
					strBuf[strLen++] = wParam;
					if (strLen == 4) {
						int weiNum = atoi(strBuf + 2);
						strBuf[2] = 0;
						int quNum = atoi(strBuf);
						sprintf(strBuf, "%c%c", quNum + 160, weiNum + 160);

						if (ldisc) {
							lpage_send(ldisc, kbd_codepage, strBuf, 2, 1);
						}
						show_mouseptr(0);
						net_pending_errors();
						memset(strBuf, 0x00, 32);
						strLen = 0;
					}
				}
				return 0;
			}

			if (cfg.allowshortcuts == 1) {
				if (wParam >= VK_F1 && wParam <= VK_F12) {
					int i = wParam - VK_F1;
					if (KeyCodeLenFXX[i] != 0) {
						memcpy(buf, KeyCodeFXX[i], KeyCodeLenFXX[i]);
						len = KeyCodeLenFXX[i];
						buf[KeyCodeLenFXX[i]] = 0;
					}
				}
			}

			if (cfg.allowfunction) {
				if (get(wParam) != 0) {
					char temp[20];
					int templen;
					len = 0;
					memset(temp, 0x00, 20);
					memset(buf, 0x00, 20);
					memcpy(temp, get(wParam), strlen(get(wParam)));
					templen = strlen(get(wParam));
					for (int j = 0; j < templen; j++) {
						if ((temp[j] == 0x5E) && (temp[j + 1]) == 0x5B) {
							j++;
							buf[len++] = 0X1B;
						} else
							buf[len++] = temp[j];
					}
					buf[len] = 0;
				}
			}

			if (len == 0) {
				len = TranslateKey(message, wParam, lParam, buf);
			}

			if (len == -1) {
				return DefWindowProc(hwnd, message, wParam, lParam);
			}
			if (len != 0) {
				/*
				 * Interrupt an ongoing paste. I'm not sure
				 * this is sensible, but for the moment it's
				 * preferable to having to faff about buffering
				 * things.
				 */
				term_nopaste (term);

				/*
				 * We need not bother about stdin backlogs
				 * here, because in GUI PuTTY we can't do
				 * anything about it anyway; there's no means
				 * of asking Windows to hold off on KEYDOWN
				 * messages. We _have_ to buffer everything
				 * we're sent.
				 */
				term_seen_key_event(term);

				if (ldisc) {
					ldisc_send_bin(ldisc, buf, len, 1);
				}
				show_mouseptr(0);
			}
		}
	}
	net_pending_errors();
	return 0;
}
LRESULT TerminalWindow::OnInputLangChange(UINT message, WPARAM wParam,
		LPARAM lParam) {
	set_input_locale((HKL) lParam);
	sys_cursor_update();
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnImeStartComposition(UINT message, WPARAM wParam,
		LPARAM lParam) {
	HIMC hImc = ImmGetContext(hwnd);
	ImmSetCompositionFont(hImc, &lfont);
	ImmReleaseContext(hwnd, hImc);
	return 0;
}

LRESULT TerminalWindow::OnImeEndComposition(UINT message, WPARAM wParam,
		LPARAM lParam) {
	return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT TerminalWindow::OnImeComposition(UINT message, WPARAM wParam,
		LPARAM lParam) {

	HIMC hIMC;
	int n;
	char *buff;

	if ((lParam & GCS_RESULTSTR) == 0) /* Composition unfinished. */
		return DefWindowProc(hwnd, message, wParam, lParam);

	hIMC = ImmGetContext(hwnd);
	n = ImmGetCompositionString(hIMC, GCS_RESULTSTR, NULL, 0);
	if (n > 0) {
		buff = snewn(n, char);
		memset(buff, 0x00, n);
		ImmGetCompositionString(hIMC, GCS_RESULTSTR, buff, n);
		/*
		 * Jaeyoun Chung reports that Korean character
		 * input doesn't work correctly if we do a single
		 * luni_send() covering the whole of buff. So
		 * instead we luni_send the characters one by one.
		 */
		term_seen_key_event (term);
		if (ldisc)
			lpage_send(ldisc, kbd_codepage, buff, n, 1);
		free(buff);
	}
	ImmReleaseContext(hwnd, hIMC);
	return 1;
}
LRESULT TerminalWindow::OnImeChar(UINT message, WPARAM wParam, LPARAM lParam) {
	if (wParam & 0xFF00) {
		char buf[2];
		buf[1] = wParam;
		buf[0] = wParam >> 8;
		term_seen_key_event (term);
		if (ldisc)
			lpage_send(ldisc, kbd_codepage, buf, 2, 1);
	} else {
		char c = (unsigned char) wParam;
		term_seen_key_event (term);
		if (ldisc)
			lpage_send(ldisc, kbd_codepage, &c, 1, 1);
	}
	return (0);
}
LRESULT TerminalWindow::OnChar(UINT message, WPARAM wParam, LPARAM lParam) {
	/*
	 * Nevertheless, we are prepared to deal with WM_CHAR
	 * messages, should they crop up. So if someone wants to
	 * post the things to us as part of a macro manoeuvre,
	 * we're ready to cope.
	 */
	char c = (unsigned char) wParam;
	term_seen_key_event (term);
	if (ldisc)
		lpage_send(ldisc, CP_ACP, &c, 1, 1);
	return 0;
}
LRESULT TerminalWindow::OnSysColorChange(UINT message, WPARAM wParam,
		LPARAM lParam) {
	if (cfg.system_colour) {
		/* Refresh palette from system colours. */
		/* XXX actually this zaps the entire palette. */
		systopalette();
		init_palette();
		/* Force a repaint of the terminal window. */
		term_invalidate (term);
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
LRESULT TerminalWindow::OnAgentCallback(UINT message, WPARAM wParam,
		LPARAM lParam) {
	struct agent_callback *c = (struct agent_callback *) lParam;
	c->callback(c->callback_ctx, c->data, c->len);
	sfree(c);
	return 0;
}
LRESULT TerminalWindow::OnMouseWheel(UINT message, WPARAM wParam,
		LPARAM lParam) {
	if (message == wm_mousewheel || message == WM_MOUSEWHEEL) {
		int shift_pressed = 0, control_pressed = 0;

		if (message == WM_MOUSEWHEEL) {
			wheel_accumulator += (short) HIWORD(wParam);
			shift_pressed = LOWORD(wParam) & MK_SHIFT;
			control_pressed = LOWORD(wParam) & MK_CONTROL;
		} else {
			BYTE keys[256];
			wheel_accumulator += (int) wParam;
			if (GetKeyboardState(keys) != 0) {
				shift_pressed = keys[VK_SHIFT] & 0x80;
				control_pressed = keys[VK_CONTROL] & 0x80;
			}
		}

		/* process events when the threshold is reached */
		while (abs(wheel_accumulator) >= WHEEL_DELTA) {
			Mouse_Button b;

			/* reduce amount for next time */
			if (wheel_accumulator > 0) {
				b = MBT_WHEEL_UP;
				wheel_accumulator -= WHEEL_DELTA;
			} else if (wheel_accumulator < 0) {
				b = MBT_WHEEL_DOWN;
				wheel_accumulator += WHEEL_DELTA;
			} else
				break;
//
			if (send_raw_mouse && !(cfg.mouse_override && shift_pressed)) {
				/* send a mouse-down followed by a mouse up */
				term_mouse(term, b, translate_button(b), MA_CLICK,
						TO_CHR_X(X_POS(lParam)), TO_CHR_Y(Y_POS(lParam)),
						shift_pressed, control_pressed, is_alt_pressed());
				term_mouse(term, b, translate_button(b), MA_RELEASE,
						TO_CHR_X(X_POS(lParam)), TO_CHR_Y(Y_POS(lParam)),
						shift_pressed, control_pressed, is_alt_pressed());
			} else {
				/* trigger a scroll */
				term_scroll(term, 0,
						b == MBT_WHEEL_UP ? -term->rows / 2 : term->rows / 2);
			}
		}
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

void TerminalWindow::init_palette(void) {
	int i;
	HDC hdc = GetDC(hwnd);
	if (hdc) {
		if (cfg.try_palette && GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) {
			/*
			 * This is a genuine case where we must use smalloc
			 * because the snew macros can't cope.
			 */
			logpal =
					(LPLOGPALETTE) smalloc(
							sizeof(*logpal) - sizeof(logpal->palPalEntry) + NALLCOLOURS * sizeof(PALETTEENTRY));
			logpal->palVersion = 0x300;
			logpal->palNumEntries = NALLCOLOURS;
			for (i = 0; i < NALLCOLOURS; i++) {
				logpal->palPalEntry[i].peRed = defpal[i].rgbtRed;
				logpal->palPalEntry[i].peGreen = defpal[i].rgbtGreen;
				logpal->palPalEntry[i].peBlue = defpal[i].rgbtBlue;
				logpal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
			}
			pal = CreatePalette(logpal);
			if (pal) {
				SelectPalette(hdc, pal, FALSE);
				RealizePalette(hdc);
				SelectPalette(hdc, (HPALETTE) GetStockObject(DEFAULT_PALETTE),
						FALSE);
			}
		}
		ReleaseDC(hwnd, hdc);
	}
	if (pal)
		for (i = 0; i < NALLCOLOURS; i++)
			colours[i] =
					PALETTERGB(defpal[i].rgbtRed, defpal[i].rgbtGreen, defpal[i].rgbtBlue);
	else
		for (i = 0; i < NALLCOLOURS; i++)
			colours[i] =
					RGB(defpal[i].rgbtRed, defpal[i].rgbtGreen, defpal[i].rgbtBlue);
}

/*
 * Translate a WM_(SYS)?KEY(UP|DOWN) message into a string of ASCII
 * codes. Returns number of bytes used, zero to drop the message,
 * -1 to forward the message to Windows, or another negative number
 * to indicate a NUL-terminated "special" string.
 */
int TerminalWindow::TranslateKey(UINT message, WPARAM wParam, LPARAM lParam,
		unsigned char *output) {
	BYTE keystate[256];
	int scan, left_alt = 0, key_down, shift_state;
	int r, i, code;
	unsigned char *p = output;
	static int alt_sum = 0;

	HKL kbd_layout = GetKeyboardLayout(0);

	/* keys is for ToAsciiEx. There's some ick here, see below. */
	static WORD keys[3];
	static int compose_char = 0;
	static WPARAM compose_key = 0;

	r = GetKeyboardState(keystate);
	if (!r)
		memset(keystate, 0, sizeof(keystate));
	else {
#if 0
#define SHOW_TOASCII_RESULT
		{ /* Tell us all about key events */
			static BYTE oldstate[256];
			static int first = 1;
			static int scan;
			int ch;
			if (first)
			memcpy(oldstate, keystate, sizeof(oldstate));
			first = 0;

			if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == KF_REPEAT) {
				debug(("+"));
			} else if ((HIWORD(lParam) & KF_UP)
					&& scan == (HIWORD(lParam) & 0xFF)) {
				debug((". U"));
			} else {
				debug((".\n"));
				if (wParam >= VK_F1 && wParam <= VK_F20)
				debug(("K_F%d", wParam + 1 - VK_F1));
				else
				switch (wParam) {
					case VK_SHIFT:
					debug(("SHIFT"));
					break;
					case VK_CONTROL:
					debug(("CTRL"));
					break;
					case VK_MENU:
					debug(("ALT"));
					break;
					default:
					debug(("VK_%02x", wParam));
				}
				if (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP)
				debug(("*"));
				debug((", S%02x", scan = (HIWORD(lParam) & 0xFF)));

				ch = MapVirtualKeyEx(wParam, 2, kbd_layout);
				if (ch >= ' ' && ch <= '~')
				debug((", '%c'", ch));
				else if (ch)
				debug((", $%02x", ch));

				if (keys[0])
				debug((", KB0=%02x", keys[0]));
				if (keys[1])
				debug((", KB1=%02x", keys[1]));
				if (keys[2])
				debug((", KB2=%02x", keys[2]));

				if ((keystate[VK_SHIFT] & 0x80) != 0)
				debug((", S"));
				if ((keystate[VK_CONTROL] & 0x80) != 0)
				debug((", C"));
				if ((HIWORD(lParam) & KF_EXTENDED))
				debug((", E"));
				if ((HIWORD(lParam) & KF_UP))
				debug((", U"));
			}

			if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == KF_REPEAT);
			else if ((HIWORD(lParam) & KF_UP))
			oldstate[wParam & 0xFF] ^= 0x80;
			else
			oldstate[wParam & 0xFF] ^= 0x81;

			for (ch = 0; ch < 256; ch++)
			if (oldstate[ch] != keystate[ch])
			debug((", M%02x=%02x", ch, keystate[ch]));

			memcpy(oldstate, keystate, sizeof(oldstate));
		}
#endif

		if (wParam == VK_MENU && (HIWORD(lParam) & KF_EXTENDED)) {
			keystate[VK_RMENU] = keystate[VK_MENU];
		}
		/* Nastyness with NUMLock - Shift-NUMLock is left alone though */
		if ((cfg.funky_type == FUNKY_VT400
				|| (cfg.funky_type <= FUNKY_LINUX && term->app_keypad_keys
						&& !cfg.no_applic_k)) && wParam == VK_NUMLOCK
				&& !(keystate[VK_SHIFT] & 0x80)) {
			wParam = VK_EXECUTE;

			/* UnToggle NUMLock */
			if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == 0)
				keystate[VK_NUMLOCK] ^= 1;
		}

		//if ((cfg.funky_type == FUNKY_VT400 ||(cfg.funky_type <= FUNKY_LINUX && term->app_keypad_keys && !cfg.no_applic_k)) && wParam == VK_BACK && !(keystate[VK_SHIFT] & 0x80))
		//{

		//    wParam = VK_EXECUTE;

		//    /* UnToggle NUMLock */
		//    if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) == 0)
		//	keystate[VK_BACK] ^= 1;
		//}
		//

		/* And write back the 'adjusted' state */
		SetKeyboardState(keystate);
	}

	/* Disable Auto repeat if required */
	if (term->repeat_off && (HIWORD(lParam) & (KF_UP | KF_REPEAT)) == KF_REPEAT)
		return 0;

	if ((HIWORD(lParam) & KF_ALTDOWN) && (keystate[VK_RMENU] & 0x80) == 0)
		left_alt = 1;

	key_down = ((HIWORD(lParam) & KF_UP) == 0);

	/* Make sure Ctrl-ALT is not the same as AltGr for ToAscii unless told. */
	if (left_alt && (keystate[VK_CONTROL] & 0x80)) {
		if (cfg.ctrlaltkeys)
			keystate[VK_MENU] = 0;
		else {
			keystate[VK_RMENU] = 0x80;
			left_alt = 0;
		}
	}

	scan = (HIWORD(lParam) & (KF_UP | KF_EXTENDED | 0xFF));
	shift_state = ((keystate[VK_SHIFT] & 0x80) != 0)
			+ ((keystate[VK_CONTROL] & 0x80) != 0) * 2;
	/* Note if AltGr was pressed and if it was used as a compose key */
	if (!compose_state) {
		compose_key = 0x100; // 走
		if (cfg.compose_key) {
			if (wParam == VK_MENU && (HIWORD(lParam) & KF_EXTENDED))
				compose_key = wParam;
		}
		if (wParam == VK_APPS)
			compose_key = wParam;
	}
	if (wParam == compose_key) {
		if (compose_state == 0 && (HIWORD(lParam) & (KF_UP | KF_REPEAT)) == 0)
			compose_state = 1;
		else if (compose_state == 1 && (HIWORD(lParam) & KF_UP))
			compose_state = 2;
		else
			compose_state = 0;
	} else if (compose_state == 1 && wParam != VK_CONTROL)
		compose_state = 0;

	if (compose_state > 1 && left_alt)
		compose_state = 0;

	/* Sanitize the number pad if not using a PC NumPad */
	if (left_alt
			|| (term->app_keypad_keys && !cfg.no_applic_k
					&& cfg.funky_type != FUNKY_XTERM)
			|| cfg.funky_type == FUNKY_VT400 || cfg.nethack_keypad
			|| compose_state) {
		if ((HIWORD(lParam) & KF_EXTENDED) == 0) {
			int nParam = 0;
			switch (wParam) {
			case VK_INSERT:
				nParam = VK_NUMPAD0;
				break;
			case VK_END:
				nParam = VK_NUMPAD1;
				break;
			case VK_DOWN:
				nParam = VK_NUMPAD2;
				break;
			case VK_NEXT:
				nParam = VK_NUMPAD3;
				break;
			case VK_LEFT:
				nParam = VK_NUMPAD4;
				break;
			case VK_CLEAR:
				nParam = VK_NUMPAD5;
				break;
			case VK_RIGHT:
				nParam = VK_NUMPAD6;
				break;
			case VK_HOME:
				nParam = VK_NUMPAD7;
				break;
			case VK_UP:
				nParam = VK_NUMPAD8;
				break;
			case VK_PRIOR:
				nParam = VK_NUMPAD9;
				break;
			case VK_DELETE:
				nParam = VK_DECIMAL;
				break;
			}
			if (nParam) {
				if (keystate[VK_NUMLOCK] & 1)
					shift_state |= 1;
				wParam = nParam;
			}
		}
	}
	/* If a key is pressed and AltGr is not active */
	if (key_down && (keystate[VK_RMENU] & 0x80) == 0 && !compose_state) {
		/* Okay, prepare for most alts then ... */
		if (left_alt)
			*p++ = '\033';

		/* Lets see if it's a pattern we know all about ... */
		if (wParam == VK_PRIOR && shift_state == 1) {
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
			return 0;
		}
		if (wParam == VK_PRIOR && shift_state == 2) {
			SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
			return 0;
		}
		if (wParam == VK_NEXT && shift_state == 1) {
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
			return 0;
		}
		if (wParam == VK_NEXT && shift_state == 2) {
			SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
			return 0;
		}
		if (wParam == VK_INSERT && shift_state == 1) {
			term_do_paste (term);
			return 0;
		}
		if (left_alt && wParam == VK_F4 && cfg.alt_f4) {
			return -1;
		}
		if (left_alt && wParam == VK_SPACE && cfg.alt_space) {
			SendMessage(hwnd, WM_SYSCOMMAND, SC_KEYMENU, 0);
			return -1;
		}
		if (left_alt && wParam == VK_RETURN && cfg.fullscreenonaltenter
				&& (cfg.resize_action != RESIZE_DISABLED)) {
			if ((HIWORD(lParam) & (KF_UP | KF_REPEAT)) != KF_REPEAT)
				flip_full_screen();
			return -1;
		}
		/* Control-Numlock for app-keypad mode switch */
		if (wParam == VK_PAUSE && shift_state == 2) {
			term->app_keypad_keys ^= 1;
			return 0;
		}

		/* Nethack keypad */
		if (cfg.nethack_keypad && !left_alt) {
			switch (wParam) {
			case VK_NUMPAD1:
				*p++ = "bB\002\002"[shift_state & 3];
				return p - output;
			case VK_NUMPAD2:
				*p++ = "jJ\012\012"[shift_state & 3];
				return p - output;
			case VK_NUMPAD3:
				*p++ = "nN\016\016"[shift_state & 3];
				return p - output;
			case VK_NUMPAD4:
				*p++ = "hH\010\010"[shift_state & 3];
				return p - output;
			case VK_NUMPAD5:
				*p++ = shift_state ? '.' : '.';
				return p - output;
			case VK_NUMPAD6:
				*p++ = "lL\014\014"[shift_state & 3];
				return p - output;
			case VK_NUMPAD7:
				*p++ = "yY\031\031"[shift_state & 3];
				return p - output;
			case VK_NUMPAD8:
				*p++ = "kK\013\013"[shift_state & 3];
				return p - output;
			case VK_NUMPAD9:
				*p++ = "uU\025\025"[shift_state & 3];
				return p - output;
			}
		}

		/* Application Keypad */
		if (!left_alt) {
			int xkey = 0;

			if (cfg.funky_type == FUNKY_VT400
					|| (cfg.funky_type <= FUNKY_LINUX && term->app_keypad_keys
							&& !cfg.no_applic_k))
				switch (wParam) {
				case VK_EXECUTE:
					xkey = 'P';
					break;
				case VK_DIVIDE:
					xkey = 'Q';
					break;
				case VK_MULTIPLY:
					xkey = 'R';
					break;
				case VK_SUBTRACT:
					xkey = 'S';
					break;
				}
			if (term->app_keypad_keys && !cfg.no_applic_k)
				switch (wParam) {
				case VK_NUMPAD0:
					xkey = 'p';
					break;
				case VK_NUMPAD1:
					xkey = 'q';
					break;
				case VK_NUMPAD2:
					xkey = 'r';
					break;
				case VK_NUMPAD3:
					xkey = 's';
					break;
				case VK_NUMPAD4:
					xkey = 't';
					break;
				case VK_NUMPAD5:
					xkey = 'u';
					break;
				case VK_NUMPAD6:
					xkey = 'v';
					break;
				case VK_NUMPAD7:
					xkey = 'w';
					break;
				case VK_NUMPAD8:
					xkey = 'x';
					break;
				case VK_NUMPAD9:
					xkey = 'y';
					break;

				case VK_DECIMAL:
					xkey = 'n';
					break;
				case VK_ADD:
					if (cfg.funky_type == FUNKY_XTERM) {
						if (shift_state)
							xkey = 'l';
						else
							xkey = 'k';
					} else if (shift_state)
						xkey = 'm';
					else
						xkey = 'l';
					break;

				case VK_DIVIDE:
					if (cfg.funky_type == FUNKY_XTERM)
						xkey = 'o';
					break;
				case VK_MULTIPLY:
					if (cfg.funky_type == FUNKY_XTERM)
						xkey = 'j';
					break;
				case VK_SUBTRACT:
					if (cfg.funky_type == FUNKY_XTERM)
						xkey = 'm';
					break;

				case VK_RETURN:
					if (HIWORD(lParam) & KF_EXTENDED)
						xkey = 'M';
					break;
				}
			if (xkey) {
				if (term->vt52_mode) {
					if (xkey >= 'P' && xkey <= 'S')
						p += sprintf((char *) p, "\x1B%c", xkey);
					else
						p += sprintf((char *) p, "\x1B?%c", xkey);
				} else
					p += sprintf((char *) p, "\x1BO%c", xkey);
				return p - output;
			}
		}

		if ((wParam == VK_DELETE) && (cfg.deleteisreturn == 1)) {
			*p++ = 0x7F;
			*p++ = 0;
			return -2;
		}
		if (wParam == VK_BACK && shift_state == 0) { /* Backspace */
			if ((cfg.bksp_is_delete == 0) || (cfg.bksp_is_delete == 1)) {
				*p++ = (cfg.bksp_is_delete ? 0x7F : 0x08);
				*p++ = 0;
				return -2;
			} else if (cfg.bksp_is_delete == 2) {
				wParam = VK_LEFT;
			}

		}
		if (wParam == VK_BACK && shift_state == 1) { /* Shift Backspace */
			/* We do the opposite of what is configured */
			*p++ = (cfg.bksp_is_delete ? 0x08 : 0x7F);
			*p++ = 0;
			return -2;
		}
		if (wParam == VK_TAB && shift_state == 1) { /* Shift tab */
			*p++ = 0x1B;
			*p++ = '[';
			*p++ = 'Z';
			return p - output;
		}
		if (wParam == VK_SPACE && shift_state == 2) { /* Ctrl-Space */
			*p++ = 0;
			return p - output;
		}
		if (wParam == VK_SPACE && shift_state == 3) { /* Ctrl-Shift-Space */
			*p++ = 160;
			return p - output;
		}
		if (wParam == VK_CANCEL && shift_state == 2) { /* Ctrl-Break */
			if (back)
				back->special(backhandle, TS_BRK);
			return 0;
		}
		if (wParam == VK_PAUSE) { /* Break/Pause */
			*p++ = 26;
			*p++ = 0;
			return -2;
		}
		/* Control-2 to Control-8 are special */
		if (shift_state == 2 && wParam >= '2' && wParam <= '8') {
			*p++ = "\000\033\034\035\036\037\177"[wParam - '2'];
			return p - output;
		}
		if (shift_state == 2 && (wParam == 0xBD || wParam == 0xBF)) {
			*p++ = 0x1F;
			return p - output;
		}
		if (shift_state == 2 && wParam == 0xDF) {
			*p++ = 0x1C;
			return p - output;
		}
		if (shift_state == 3 && wParam == 0xDE) {
			*p++ = 0x1E; /* Ctrl-~ == Ctrl-^ in xterm at least */
			return p - output;
		}
		if (shift_state == 0 && wParam == VK_RETURN && term->cr_lf_return) {
			*p++ = '\r';
			*p++ = '\n';
			return p - output;
		}

		/*
		 * Next, all the keys that do tilde codes. (ESC '[' nn '~',
		 * for integer decimal nn.)
		 *
		 * We also deal with the weird ones here. Linux VCs replace F1
		 * to F5 by ESC [ [ A to ESC [ [ E. rxvt doesn't do _that_, but
		 * does replace Home and End (1~ and 4~) by ESC [ H and ESC O w
		 * respectively.
		 */
		code = 0;
		switch (wParam) {
		case VK_F1:
			code = (keystate[VK_SHIFT] & 0x80 ? 23 : 11);
			break;
		case VK_F2:
			code = (keystate[VK_SHIFT] & 0x80 ? 24 : 12);
			break;
		case VK_F3:
			code = (keystate[VK_SHIFT] & 0x80 ? 25 : 13);
			break;
		case VK_F4:
			code = (keystate[VK_SHIFT] & 0x80 ? 26 : 14);
			break;
		case VK_F5:
			code = (keystate[VK_SHIFT] & 0x80 ? 28 : 15);
			break;
		case VK_F6:
			code = (keystate[VK_SHIFT] & 0x80 ? 29 : 17);
			break;
		case VK_F7:
			code = (keystate[VK_SHIFT] & 0x80 ? 31 : 18);
			break;
		case VK_F8:
			code = (keystate[VK_SHIFT] & 0x80 ? 32 : 19);
			break;
		case VK_F9:
			code = (keystate[VK_SHIFT] & 0x80 ? 33 : 20);
			break;
		case VK_F10:
			code = (keystate[VK_SHIFT] & 0x80 ? 34 : 21);
			break;
		case VK_F11:
			code = 23;
			break;
		case VK_F12:
			code = 24;
			break;
		case VK_F13:
			code = 25;
			break;
		case VK_F14:
			code = 26;
			break;
		case VK_F15:
			code = 28;
			break;
		case VK_F16:
			code = 29;
			break;
		case VK_F17:
			code = 31;
			break;
		case VK_F18:
			code = 32;
			break;
		case VK_F19:
			code = 33;
			break;
		case VK_F20:
			code = 34;
			break;
		}
		if ((shift_state & 2) == 0)
			switch (wParam) {
			case VK_HOME:
				code = 1;
				break;
			case VK_INSERT:
				code = 2;
				break;
			case VK_DELETE:
				code = 3;
				break;
			case VK_END:
				code = 4;
				break;
			case VK_PRIOR:
				code = 5;
				break;
			case VK_NEXT:
				code = 6;
				break;
			}
		/* Reorder edit keys to physical order */
		if (cfg.funky_type == FUNKY_VT400 && code <= 6)
			code = "\0\2\1\4\5\3\6"[code];

		if (term->vt52_mode && code > 0 && code <= 6) {
			p += sprintf((char *) p, "\x1B%c", " HLMEIG"[code]);
			return p - output;
		}

		if (cfg.funky_type == FUNKY_SCO && /* SCO function keys */
		code >= 11 && code <= 34) {
			char codes[] = "MNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@[\\]^_`{";
			int index = 0;
			switch (wParam) {
			case VK_F1:
				index = 0;
				break;
			case VK_F2:
				index = 1;
				break;
			case VK_F3:
				index = 2;
				break;
			case VK_F4:
				index = 3;
				break;
			case VK_F5:
				index = 4;
				break;
			case VK_F6:
				index = 5;
				break;
			case VK_F7:
				index = 6;
				break;
			case VK_F8:
				index = 7;
				break;
			case VK_F9:
				index = 8;
				break;
			case VK_F10:
				index = 9;
				break;
			case VK_F11:
				index = 10;
				break;
			case VK_F12:
				index = 11;
				break;
			}
			if (keystate[VK_SHIFT] & 0x80)
				index += 12;
			if (keystate[VK_CONTROL] & 0x80)
				index += 24;
			p += sprintf((char *) p, "\x1B[%c", codes[index]);
			return p - output;
		}
		if (cfg.funky_type == FUNKY_SCO && /* SCO small keypad */
		code >= 1 && code <= 6) {
			char codes[] = "HL.FIG";
			if (code == 3) {
				*p++ = '\x7F';
			} else {
				p += sprintf((char *) p, "\x1B[%c", codes[code - 1]);
			}
			return p - output;
		}
		if ((term->vt52_mode || cfg.funky_type == FUNKY_VT100P) && code >= 11
				&& code <= 24) {
			int offt = 0;
			if (code > 15)
				offt++;
			if (code > 21)
				offt++;
			if (term->vt52_mode)
				p += sprintf((char *) p, "\x1B%c", code + 'P' - 11 - offt);
			else
				p += sprintf((char *) p, "\x1BO%c", code + 'P' - 11 - offt);
			return p - output;
		}
		if (cfg.funky_type == FUNKY_LINUX && code >= 11 && code <= 15) {
			p += sprintf((char *) p, "\x1B[[%c", code + 'A' - 11);
			return p - output;
		}
		if (cfg.funky_type == FUNKY_XTERM && code >= 11 && code <= 14) {
			if (term->vt52_mode)
				p += sprintf((char *) p, "\x1B%c", code + 'P' - 11);
			else
				p += sprintf((char *) p, "\x1BO%c", code + 'P' - 11);
			return p - output;
		}
		if (cfg.rxvt_homeend && (code == 1 || code == 4)) {
			p += sprintf((char *) p, code == 1 ? "\x1B[H" : "\x1BOw");
			return p - output;
		}
		if (code) {
			p += sprintf((char *) p, "\x1B[%d~", code);
			return p - output;
		}

		/*
		 * Now the remaining keys (arrows and Keypad 5. Keypad 5 for
		 * some reason seems to send VK_CLEAR to Windows...).
		 */
		//左键
		{
			char xkey = 0;
			switch (wParam) {
			case VK_UP:
				xkey = 'A';
				break;
			case VK_DOWN:
				xkey = 'B';
				break;
			case VK_RIGHT:
				xkey = 'C';
				break;
			case VK_LEFT:
				xkey = 'D';
				break;
			case VK_CLEAR:
				xkey = 'G';
				break;
			}
			if (xkey) {
				if (term->vt52_mode)
					p += sprintf((char *) p, "\x1B%c", xkey);
				else {
					int app_flg = (term->app_cursor_keys && !cfg.no_applic_c);
#if 0
					/*
					 * RDB: VT100 & VT102 manuals both state the
					 * app cursor keys only work if the app keypad
					 * is on.
					 *
					 * SGT: That may well be true, but xterm
					 * disagrees and so does at least one
					 * application, so I've #if'ed this out and the
					 * behaviour is back to PuTTY's original: app
					 * cursor and app keypad are independently
					 * switchable modes. If anyone complains about
					 * _this_ I'll have to put in a configurable
					 * option.
					 */
					if (!term->app_keypad_keys)
					app_flg = 0;
#endif
					/* Useful mapping of Ctrl-arrows */
					if (shift_state == 2)
						app_flg = !app_flg;

					if (app_flg)
						p += sprintf((char *) p, "\x1BO%c", xkey);
					else
						p += sprintf((char *) p, "\x1B[%c", xkey);
				}
				return p - output;
			}
		}

		/*
		 * Finally, deal with Return ourselves. (Win95 seems to
		 * foul it up when Alt is pressed, for some reason.)
		 */
		if (wParam == VK_RETURN) { /* Return */
			*p++ = 0x0D;
			*p++ = 0;
			return -2;
		}

		if (left_alt && wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
			alt_sum = alt_sum * 10 + wParam - VK_NUMPAD0;
		else
			alt_sum = 0;
	}

	/* Okay we've done everything interesting; let windows deal with
	 * the boring stuff */
	{
		BOOL capsOn = 0;

		/* helg: clear CAPS LOCK state if caps lock switches to cyrillic */
		if (cfg.xlat_capslockcyr && keystate[VK_CAPITAL] != 0) {
			capsOn = !left_alt;
			keystate[VK_CAPITAL] = 0;
		}

		/* XXX how do we know what the max size of the keys array should
		 * be is? There's indication on MS' website of an Inquire/InquireEx
		 * functioning returning a KBINFO structure which tells us. */
		if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			/* XXX 'keys' parameter is declared in MSDN documentation as
			 * 'LPWORD lpChar'.
			 * The experience of a French user indicates that on
			 * Win98, WORD[] should be passed in, but on Win2K, it should
			 * be BYTE[]. German WinXP and my Win2K with "US International"
			 * driver corroborate this.
			 * Experimentally I've conditionalised the behaviour on the
			 * Win9x/NT split, but I suspect it's worse than that.
			 * See wishlist item `win-dead-keys' for more horrible detail
			 * and speculations. */
			BYTE keybs[3];
			int i;
			r = ToAsciiEx(wParam, scan, keystate, (LPWORD) keybs, 0,
					kbd_layout);
			for (i = 0; i < 3; i++)
				keys[i] = keybs[i];
		} else {
			r = ToAsciiEx(wParam, scan, keystate, keys, 0, kbd_layout);
		}
#ifdef SHOW_TOASCII_RESULT
		if (r == 1 && !key_down) {
			if (alt_sum) {
				if (in_utf(term) || ucsdata.dbcs_screenfont)
				debug((", (U+%04x)", alt_sum));
				else
				debug((", LCH(%d)", alt_sum));
			} else {
				debug((", ACH(%d)", keys[0]));
			}
		} else if (r > 0) {
			int r1;
			debug((", ASC("));
			for (r1 = 0; r1 < r; r1++) {
				debug(("%s%d", r1 ? "," : "", keys[r1]));
			}
			debug((")"));
		}
#endif
		if (r > 0) {
			WCHAR keybuf;

			/*
			 * Interrupt an ongoing paste. I'm not sure this is
			 * sensible, but for the moment it's preferable to
			 * having to faff about buffering things.
			 */
			term_nopaste (term);

			p = output;
			for (i = 0; i < r; i++) {
				unsigned char ch = (unsigned char) keys[i];

				if (compose_state == 2 && (ch & 0x80) == 0 && ch > ' ') {
					compose_char = ch;
					compose_state++;
					continue;
				}
				if (compose_state == 3 && (ch & 0x80) == 0 && ch > ' ') {
					int nc;
					compose_state = 0;

					if ((nc = check_compose(compose_char, ch)) == -1) {
						MessageBeep(MB_ICONHAND);
						return 0;
					}
					keybuf = nc;
					term_seen_key_event(term);
					if (ldisc)
						luni_send(ldisc, &keybuf, 1, 1);
					continue;
				}

				compose_state = 0;

				if (!key_down) {
					if (alt_sum) {
						if (in_utf(term) || ucsdata.dbcs_screenfont) {
							keybuf = alt_sum;
							term_seen_key_event(term);
							if (ldisc)
								luni_send(ldisc, &keybuf, 1, 1);
						} else {
							ch = (char) alt_sum;
							/*
							 * We need not bother about stdin
							 * backlogs here, because in GUI PuTTY
							 * we can't do anything about it
							 * anyway; there's no means of asking
							 * Windows to hold off on KEYDOWN
							 * messages. We _have_ to buffer
							 * everything we're sent.
							 */
							term_seen_key_event(term);
							if (ldisc)
								ldisc_send(ldisc, (char*) &ch, 1, 1);
						}
						alt_sum = 0;
					} else {
						term_seen_key_event(term);
						if (ldisc)
							lpage_send(ldisc, kbd_codepage, (char*) &ch, 1, 1);
					}
				} else {
					if (capsOn && ch < 0x80) {
						WCHAR cbuf[2];
						cbuf[0] = 27;
						cbuf[1] = xlat_uskbd2cyrllic(ch);
						term_seen_key_event(term);
						if (ldisc)
							luni_send(ldisc, cbuf + !left_alt, 1 + !!left_alt,
									1);
					} else {
						char cbuf[2];
						cbuf[0] = '\033';
						cbuf[1] = ch;
						term_seen_key_event(term);
						if (ldisc)
							lpage_send(ldisc, kbd_codepage, cbuf + !left_alt,
									1 + !!left_alt, 1);
					}
				}
				show_mouseptr(0);
			}

			/* This is so the ALT-Numpad and dead keys work correctly. */
			keys[0] = 0;

			return p - output;
		} else {
			keys[0] = 0;
		}
		/* If we're definitly not building up an ALT-54321 then clear it */
		if (!left_alt)
			keys[0] = 0;
		/* If we will be using alt_sum fix the 256s */
		else if (keys[0] && (in_utf(term) || ucsdata.dbcs_screenfont))
			keys[0] = 10;
	}

	/*
	 * ALT alone may or may not want to bring up the System menu.
	 * If it's not meant to, we return 0 on presses or releases of
	 * ALT, to show that we've swallowed the keystroke. Otherwise
	 * we return -1, which means Windows will give the keystroke
	 * its default handling (i.e. bring up the System menu).
	 */
	if (wParam == VK_MENU && !cfg.alt_only)
		return 0;

	return -1;
}

} /* namespace terminal */

using namespace terminal;

#ifdef DEBUG_MAIN
#include "../printapi.h"

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show) {
	void* printer = print_new("LPT1", PRINT_LPT, 0);

	log_open();
	log_set_encrypt(FALSE);

	char buf[1024];
	print_open(printer);
	print_file(printer, "print.txt");
//	for (int i=0; i< 10; i++){
//		strcpy(buf, APPLICATION_NAME "df \n");
//		print_data(printer, buf, strlen(buf));
//	}
//	strcpy(buf, "a2bc");
//	print_data(printer, buf, strlen(buf));
//	strcpy(buf, "3abc");
//	print_data(printer, buf, strlen(buf));
	print_close(printer);

//	print_open(printer);
//	strcpy(buf, "abc1");
//	print_data(printer, buf, strlen(buf));
//	strcpy(buf, "a2bc");
//	print_data(printer, buf, strlen(buf));
//	strcpy(buf, "222c");
//	buf[3] = 12;
//	print_data(printer, buf, strlen(buf));
//	print_close(printer);

	MessageBox(NULL, "123", "abc", 1);

	print_del(printer);

	log_close();
	return 0;
}

#else // not define ZHANGBO_DEBUG

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show) {

	hMutex = CreateMutex(NULL, FALSE, "terminal");
	DWORD lasterror = GetLastError();
	if (strlen(cmdline) == 0) {
		strcpy(sectionName, "default");
	} else {
		strcpy(sectionName, cmdline);
	}
	Util::GetCurrentPath(currentpath);
	sprintf(configpath, "%s\\config.txt", currentpath);
	Setting::LoadConfigsFile(configpath);
	default_protocol = be_default_protocol;
	default_port = 23;

	char filepath[1024];
	sprintf(filepath, "%s\\%s.txt", currentpath, sectionName);
	Setting::LoadPropertiesFile(filepath, &cfg);

	if ((lasterror == ERROR_ALREADY_EXISTS) && (cfg.allowrepeat == 0)) {
		Util::MaxWindow(APPLICATION_NAME);
		CloseHandle (hMutex);
		return 0;
	} else {
		if (cfg.printmode == 2) {
			if (cfg.allowrepeat == 0) {
				if (Util::ProcessCount("service.exe") > 0) {
					Util::KillProcess("service.exe");
					Sleep(500);
				}
			} else if (cfg.allowrepeat == 1) {
				if (Util::ProcessCount("service.exe") == 0) {
					char exepath[MAX_PATH];
					sprintf(exepath, "%s\\service.exe", currentpath);
					if (Util::FileExists(exepath)) {
						Util::ShellExecuteNoWait(exepath);
					}
				}
			}
		} else if (cfg.printmode == 3) {
			FILE* fp = fopen(cfg.fprintname, "r");
			if (fp == NULL) {
				string str = "net use ";
				str += cfg.fprintname;
				//string cmd1 = str + " /delete";
				string cmd2 = str + " \\\\127.0.0.1\\";
				cmd2 = cmd2 + cfg.fpshare + " /persistent:yes";
				//system(cmd1.c_str());
				system(cmd2.c_str());
			} else {
				fclose(fp);
			}
		}
		if (Util::ProcessCount("update.exe") == 0) {
			char exepath[MAX_PATH];
			sprintf(exepath, "%s\\update.exe", currentpath);
			if (Util::FileExists(exepath))
				Util::ShellExecuteNoWait(exepath);
		}
		DWORD NAMELEN = 128;
		GetUserName(sysusername, &NAMELEN);
		TerminalWindow::Run(inst);
	}
	CloseHandle (hMutex);
	return 0;
}
#endif //ZHANGBO_DEBUG
