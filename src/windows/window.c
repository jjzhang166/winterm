/*
 * window.c
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
#include <richedit.h>
#include "../resources/resource.h"

#define PUTTY_DO_GLOBALS
#include "../putty.h"
#include "../terminal.h"
#include "message.h"
#include "window.h"
#include "../pairs.h"
//extern int isBufferPrint;
//int IsPrintScreen = 0;
//int isReflash = 0; //是否刷新界面
//int AllLock = 0;

char *window_name, *icon_name;

/* Window layout information */
int extra_width, extra_height;
int font_width, font_height, font_dualwidth;
int offset_width, offset_height;

int IsNewConnection;

COLORREF colours[NALLCOLOURS];
HPALETTE pal;
LPLOGPALETTE logpal;
RGBTRIPLE defpal[NALLCOLOURS];

//定义各按键，键值
unsigned char KeyCodeFXX[12][8];
int KeyCodeLenFXX[12];

int comHandle[4];
int must_close_session, session_closed;
Backend *back;
void *backhandle;
int send_raw_mouse = 0;
const struct telnet_special *specials = NULL;
HMENU specials_menu = NULL;
int n_specials = 0;
struct _popup_menus popup_menus[2];
LOGFONT lfont;
struct unicode_data ucsdata;

static int descent;
static HFONT fonts[FONT_MAXNO];
static int fontflag[FONT_MAXNO];
static enum {
	BOLD_COLOURS, BOLD_SHADOW, BOLD_FONT
} bold_mode;
static enum {
	UND_LINE, UND_FONT
} und_mode;

extern char sectionName[256];
extern void flash_window(int mode);
//extern void ShowTip(HWND hwnd);

int get_fullscreen_rect(RECT * ss);
/* Dummy routine, only required in plink. */
void ldisc_update(void *frontend, int echo, int edit) {
}

void Update_KeyPairs() {
	put(VK_ESCAPE, cfg.keycodeesc);
	put(VK_INSERT, cfg.keycodeinsert);
	put(VK_DELETE, cfg.keycodedelete);
	put(VK_HOME, cfg.keycodehome);
	put(VK_SPACE, cfg.keycodespace);
	put(VK_NUMLOCK, cfg.keycodenumlock);
	put(VK_PRINT, cfg.keycodeprintscreen);
	put(VK_SCROLL, cfg.keycodescroll);
	put(VK_END, cfg.keycodeend);
	put(VK_PRIOR, cfg.keycodepageup);
	put(VK_NEXT, cfg.keycodepagedown);
	put(VK_ADD, cfg.keycodeadd);
	put(VK_SUBTRACT, cfg.keycodesub);
	put(VK_MULTIPLY, cfg.keycodemult);
	put(VK_DIVIDE, cfg.keycodediv);
	put(VK_PAUSE, cfg.keycodepause);
	put(VK_UP, cfg.keycodeup);
	put(VK_DOWN, cfg.keycodedown);
	put(VK_LEFT, cfg.keycodeleft);
	put(VK_RIGHT, cfg.keycoderight);
}

/*
 * Update the Special Commands submenu.
 */
void update_specials_menu(void *frontend) {
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

int busy_status = BUSY_NOT;
static void update_mouse_pointer(void) {
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
		//assert(0);
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

void set_busy_status(void *frontend, int status) {
	busy_status = status;
	update_mouse_pointer();
}

/*
 * set or clear the "raw mouse message" mode
 */
void set_raw_mouse_mode(void *frontend, int activate) {
	activate = activate && !cfg.no_mouse_rep;
	send_raw_mouse = activate;
	update_mouse_pointer();
}

#define TIMING_TIMER_ID 1234
static long timing_next_time;
void timer_change_notify(long next) {
	long ticks = next - GETTICKCOUNT();
	if (ticks <= 0)
		ticks = 1; /* just in case */
	KillTimer(hwnd, TIMING_TIMER_ID);
	SetTimer(hwnd, TIMING_TIMER_ID, ticks, NULL);
	timing_next_time = next;
}

int caret_x = -1, caret_y = -1;

void sys_cursor_update(void) {
	COMPOSITIONFORM cf;
	HIMC hIMC;

	if (!term->has_focus)
		return;

	if (caret_x < 0 || caret_y < 0)
		return;

	SetCaretPos(caret_x, caret_y);

	/* IMM calls on Win98 and beyond only */
	if (osVersion.dwPlatformId == VER_PLATFORM_WIN32s)
		return; /* 3.11 */

	if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
			&& osVersion.dwMinorVersion == 0)
		return; /* 95 */

	/* we should have the IMM functions */
	hIMC = ImmGetContext(hwnd);
	cf.dwStyle = CFS_POINT;
	cf.ptCurrentPos.x = caret_x;
	cf.ptCurrentPos.y = caret_y;
	ImmSetCompositionWindow(hIMC, &cf);

	ImmReleaseContext(hwnd, hIMC);
}
/*
 * Move the system caret. (We maintain one, even though it's
 * invisible, for the benefit of blind people: apparently some
 * helper software tracks the system caret, so we should arrange to
 * have one.)
 */
void sys_cursor(void *frontend, int x, int y) {
	int cx, cy;

	if (!term->has_focus)
		return;

	/*
	 * Avoid gratuitously re-updating the cursor position and IMM
	 * window if there's no actual change required.
	 */
	cx = x * font_width + offset_width;
	cy = y * font_height + offset_height;
	if (cx == caret_x && cy == caret_y)
		return;
	caret_x = cx;
	caret_y = cy;

	sys_cursor_update();
}

/*
 * Print a message box and close the connection.
 */
void connection_fatal(void *frontend, char *fmt, ...) {
	va_list ap;
	char *stuff;

	va_start(ap, fmt);
	stuff = dupvprintf(fmt, ap);
	va_end(ap);
//	sprintf(morestuff, "%.70s %s!", appname, FINDERROR);
//	MessageBox(hwnd, stuff, morestuff, MB_ICONERROR | MB_OK);
	term_data(term, 0, stuff, strlen(stuff));
	sfree(stuff);

	if (cfg.close_on_exit == FORCE_ON)
		PostQuitMessage(1);
	else {
		must_close_session = TRUE;
	}
}

static void deinit_fonts(void) {
	int i;
	for (i = 0; i < FONT_MAXNO; i++) {
		if (fonts[i])
			DeleteObject(fonts[i]);
		fonts[i] = 0;
		fontflag[i] = 0;
	}
}

void notify_remote_exit(void *fe) {
	int exitcode;

	char b[2048];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (!session_closed && (exitcode = back->exitcode(backhandle)) >= 0) {
		/* Abnormal exits will already have set session_closed and taken
		 * appropriate action. */

		if (cfg.close_on_exit == FORCE_ON
				|| (cfg.close_on_exit == AUTO && exitcode != INT_MAX)) {
			PostQuitMessage(0);
		} else {
			must_close_session = TRUE;
			session_closed = TRUE;

			/* exitcode == INT_MAX indicates that the connection was closed
			 * by a fatal error, so an error box will be coming our way and
			 * we should not generate this informational one. */
//			ShowTip(hwnd);
//			if (IsNewConnection == 1) {
//				CloseHandle(hMutex);
//				//重新打开一个终端
//				GetModuleFileName(NULL, b, sizeof(b) - 1);
//				sprintf(b, "%s %s", b, sectionName);
//				si.cb = sizeof(si);
//				si.lpReserved = NULL;
//				si.lpDesktop = NULL;
//				si.lpTitle = NULL;
//				si.dwFlags = 0;
//				si.cbReserved2 = 0;
//				si.lpReserved2 = NULL;
//				FILE * pfile = fopen("c:\\123.txt", "a+");
//				fwrite(b, strlen(b), 1, pfile);
//
//				fclose(pfile);
//				CreateProcess(NULL, b, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
//						NULL, NULL, &si, &pi);
//			} else {
//				BeforClose(term);
//				exit(0);
//			}
//			BeforClose(term);
//			exit(1);
		}
	}
}

static int ahextoi(char* p) {
	int n = 0;
	char* q = p;

	/* reach its tail */
	while (*q)
		q++;
	if (*p == '0' && *(p + 1) != 0)
		/* skip "0x" or "0X" */
		p += 2;
	while (*p) {
		int c;
		if (*p >= '0' && *p <= '9')
			c = *p - '0';
		else if (*p >= 'A' && *p <= 'F')
			c = *p - 'A' + 0xA;
		else if (*p >= 'a' && *p <= 'f')
			c = *p - 'a' + 0xA;
		else
			/* invalid char */
			return 0;

		n += c << ((int) (q - p - 1) * 4);
		p++;
	}
	return n;
}

int tranKeyCode(char *keyStr, unsigned char *keyCodeStr) {
	char *ptr = NULL;
	int i = 0, j = 0;
	int ptrLen = strlen(keyStr);

	char strChar[8];

	ptr = keyStr;
	for (i = 0; i < (ptrLen / 2); i++) {
		memset(strChar, 0x00, 8);
		sprintf(strChar, "0x");
		memcpy(strChar + 2, ptr, 2);
		keyCodeStr[j] = (char) ahextoi(strChar);
		j++;
		ptr = ptr + 2;
	}
	return 0;
}

/*
 * Initialise all the fonts we will need initially. There may be as many as
 * three or as few as one.  The other (poentially) twentyone fonts are done
 * if/when they are needed.
 *
 * We also:
 *
 * - check the font width and height, correcting our guesses if
 *   necessary.
 *
 * - verify that the bold font is the same width as the ordinary
 *   one, and engage shadow bolding if not.
 *
 * - verify that the underlined font is the same width as the
 *   ordinary one (manual underlining by means of line drawing can
 *   be done in a pinch).
 */
void init_fonts(int pick_width, int pick_height) {
	TEXTMETRIC tm;
	CPINFO cpinfo;
	int fontsize[3];
	int i;
	HDC hdc;
	int fw_dontcare, fw_bold;

	for (i = 0; i < FONT_MAXNO; i++)
		fonts[i] = NULL;

	bold_mode = cfg.bold_colour ? BOLD_COLOURS : BOLD_FONT;
	und_mode = UND_FONT;

	if (cfg.font.isbold) {
		fw_dontcare = FW_BOLD;
		fw_bold = FW_HEAVY;
	} else {
		fw_dontcare = FW_DONTCARE;
		fw_bold = FW_BOLD;
	}

	hdc = GetDC(hwnd);

	if (pick_height)
		font_height = pick_height;
	else {
		font_height = cfg.font.height;
		if (font_height > 0) {
			font_height = -MulDiv(font_height, GetDeviceCaps(hdc, LOGPIXELSY),
					72);
		}
	}
	font_width = pick_width;

#define f(i,c,w,u) \
    fonts[i] = CreateFont (font_height, font_width, 0, 0, w, FALSE, u, FALSE, \
			   c, OUT_DEFAULT_PRECIS, \
		           CLIP_DEFAULT_PRECIS, FONT_QUALITY(cfg.font_quality), \
			   FIXED_PITCH | FF_DONTCARE, cfg.font.name)

	f(FONT_NORMAL, cfg.font.charset, fw_dontcare, FALSE);

	SelectObject(hdc, fonts[FONT_NORMAL]);
	GetTextMetrics(hdc, &tm);

	GetObject(fonts[FONT_NORMAL], sizeof(LOGFONT), &lfont);

	if (pick_width == 0 || pick_height == 0) {
		font_height = tm.tmHeight;
		font_width = tm.tmAveCharWidth;
	}
	font_dualwidth = (tm.tmAveCharWidth != tm.tmMaxCharWidth);

#ifdef RDB_DEBUG_PATCH
	debug(23, "Primary font H=%d, AW=%d, MW=%d",
			tm.tmHeight, tm.tmAveCharWidth, tm.tmMaxCharWidth);
#endif

	{
		CHARSETINFO info;
		DWORD cset = tm.tmCharSet;
		memset(&info, 0xFF, sizeof(info));

		/* !!! Yes the next line is right */
		if (cset == OEM_CHARSET)
			ucsdata.font_codepage = GetOEMCP();
		else if (TranslateCharsetInfo((DWORD *) cset, &info, TCI_SRCCHARSET))
			ucsdata.font_codepage = info.ciACP;
		else
			ucsdata.font_codepage = -1;

		GetCPInfo(ucsdata.font_codepage, &cpinfo);
		ucsdata.dbcs_screenfont = (cpinfo.MaxCharSize > 1);
	}

	f(FONT_UNDERLINE, cfg.font.charset, fw_dontcare, TRUE);

	/*
	 * Some fonts, e.g. 9-pt Courier, draw their underlines
	 * outside their character cell. We successfully prevent
	 * screen corruption by clipping the text output, but then
	 * we lose the underline completely. Here we try to work
	 * out whether this is such a font, and if it is, we set a
	 * flag that causes underlines to be drawn by hand.
	 *
	 * Having tried other more sophisticated approaches (such
	 * as examining the TEXTMETRIC structure or requesting the
	 * height of a string), I think we'll do this the brute
	 * force way: we create a small bitmap, draw an underlined
	 * space on it, and test to see whether any pixels are
	 * foreground-coloured. (Since we expect the underline to
	 * go all the way across the character cell, we only search
	 * down a single column of the bitmap, half way across.)
	 */
	{
		HDC und_dc;
		HBITMAP und_bm, und_oldbm;
		int i, gotit;
		COLORREF c;

		und_dc = CreateCompatibleDC(hdc);
		und_bm = CreateCompatibleBitmap(hdc, font_width, font_height);
		und_oldbm = SelectObject(und_dc, und_bm);
		SelectObject(und_dc, fonts[FONT_UNDERLINE]);
		SetTextAlign(und_dc, TA_TOP | TA_LEFT | TA_NOUPDATECP);
		SetTextColor(und_dc, RGB(255, 255, 255));
		SetBkColor(und_dc, RGB(0, 0, 0));
		SetBkMode(und_dc, OPAQUE);
		ExtTextOut(und_dc, 0, 0, ETO_OPAQUE, NULL, " ", 1, NULL);
		gotit = FALSE;
		for (i = 0; i < font_height; i++) {
			c = GetPixel(und_dc, font_width / 2, i);
			if (c != RGB(0, 0, 0))
				gotit = TRUE;
		}
		SelectObject(und_dc, und_oldbm);
		DeleteObject(und_bm);
		DeleteDC(und_dc);
		if (!gotit) {
			und_mode = UND_LINE;
			DeleteObject(fonts[FONT_UNDERLINE]);
			fonts[FONT_UNDERLINE] = 0;
		}
	}

	if (bold_mode == BOLD_FONT) {
		f(FONT_BOLD, cfg.font.charset, fw_bold, FALSE);
	}
#undef f

	descent = tm.tmAscent + 1;
	if (descent >= font_height)
		descent = font_height - 1;

	for (i = 0; i < 3; i++) {
		if (fonts[i]) {
			if (SelectObject(hdc, fonts[i]) && GetTextMetrics(hdc, &tm))
				fontsize[i] = tm.tmAveCharWidth + 256 * tm.tmHeight;
			else
				fontsize[i] = -i;
		} else
			fontsize[i] = -i;
	}

	ReleaseDC(hwnd, hdc);

	if (fontsize[FONT_UNDERLINE] != fontsize[FONT_NORMAL]) {
		und_mode = UND_LINE;
		DeleteObject(fonts[FONT_UNDERLINE]);
		fonts[FONT_UNDERLINE] = 0;
	}

	if (bold_mode == BOLD_FONT
			&& fontsize[FONT_BOLD] != fontsize[FONT_NORMAL]) {
		bold_mode = BOLD_SHADOW;
		DeleteObject(fonts[FONT_BOLD]);
		fonts[FONT_BOLD] = 0;
	}
	fontflag[0] = fontflag[1] = fontflag[2] = 1;

	init_ucs(&cfg, &ucsdata);
}

void reset_window(int reinit) {
	/*
	 * This function decides how to resize or redraw when the
	 * user changes something.
	 *
	 * This function doesn't like to change the terminal size but if the
	 * font size is locked that may be it's only soluion.
	 */
	int fHeight = 0;
	int fWidth = 0;

	int win_width, win_height;
	RECT cr, wr;

#ifdef RDB_DEBUG_PATCH
	debug((27, "reset_window()"));
#endif

	/* Current window sizes ... */
	GetWindowRect(hwnd, &wr);
	GetClientRect(hwnd, &cr);

	win_width = cr.right - cr.left;
	win_height = cr.bottom - cr.top;

	if (cfg.resize_action == RESIZE_DISABLED)
		reinit = 2;

	/* Are we being forced to reload the fonts ? */
	if (reinit > 1) {
#ifdef RDB_DEBUG_PATCH
		debug((27, "reset_window() -- Forced deinit"));
#endif
		deinit_fonts();
//		init_fonts(cfg.fontwidth, cfg.fontheight);
	}

	/* Oh, looks like we're minimised */
	if (win_width == 0 || win_height == 0)
		return;

	/* Is the window out of position ? */
	if (!reinit
			&& (offset_width != (win_width - font_width * term->cols) / 2
					|| offset_height
							!= (win_height - font_height * term->rows) / 2)) {
		offset_width = (win_width - font_width * term->cols) / 2;
		offset_height = (win_height - font_height * term->rows) / 2;
		InvalidateRect(hwnd, NULL, TRUE);
#ifdef RDB_DEBUG_PATCH
		debug((27, "reset_window() -> Reposition terminal"));
#endif
	}

	if (IsZoomed(hwnd)) {
		/* We're fullscreen, this means we must not change the size of
		 * the window so it's the font size or the terminal itself.
		 */

		extra_width = wr.right - wr.left - cr.right + cr.left;
		extra_height = wr.bottom - wr.top - cr.bottom + cr.top;

		if (cfg.resize_action != RESIZE_TERM) {
			if (font_width != win_width / term->cols
					|| font_height != win_height / term->rows) {
				deinit_fonts();
				init_fonts(win_width / term->cols, win_height / term->rows);
				offset_width = (win_width - font_width * term->cols) / 2;
				offset_height = (win_height - font_height * term->rows) / 2;
				InvalidateRect(hwnd, NULL, TRUE);
#ifdef RDB_DEBUG_PATCH
				debug((25, "reset_window() -> Z font resize to (%d, %d)",
								font_width, font_height));
#endif
			}
		} else {
			if (font_width * term->cols != win_width
					|| font_height * term->rows != win_height) {
				/* Our only choice at this point is to change the
				 * size of the terminal; Oh well.
				 */
				term_size(term, win_height / font_height,
						win_width / font_width, cfg.savelines);
				offset_width = (win_width - font_width * term->cols) / 2;
				offset_height = (win_height - font_height * term->rows) / 2;
				InvalidateRect(hwnd, NULL, TRUE);
#ifdef RDB_DEBUG_PATCH
				debug((27, "reset_window() -> Zoomed term_size"));
#endif
			}
		}
		return;
	}

	/* Hmm, a force re-init means we should ignore the current window
	 * so we resize to the default font size.
	 */
	if (reinit > 0) {
#ifdef RDB_DEBUG_PATCH
		debug((27, "reset_window() -> Forced re-init"));
#endif

		offset_width = offset_height = cfg.window_border;
		extra_width = wr.right - wr.left - cr.right + cr.left
				+ offset_width * 2;
		extra_height = wr.bottom - wr.top - cr.bottom + cr.top
				+ offset_height * 2;

		if (win_width != font_width * term->cols + offset_width * 2
				|| win_height != font_height * term->rows + offset_height * 2) {

			/* If this is too large windows will resize it to the maximum
			 * allowed window size, we will then be back in here and resize
			 * the font or terminal to fit.
			 */
			SetWindowPos(hwnd, NULL, 0, 0,
					font_width * term->cols + extra_width,
					font_height * term->rows + extra_height,
					SWP_NOMOVE | SWP_NOZORDER);
		}

		InvalidateRect(hwnd, NULL, TRUE);
		return;
	}

	/* Okay the user doesn't want us to change the font so we try the
	 * window. But that may be too big for the screen which forces us
	 * to change the terminal.
	 */
	if ((cfg.resize_action == RESIZE_TERM && reinit <= 0)
			|| (cfg.resize_action == RESIZE_EITHER && reinit < 0)
			|| reinit > 0) {
		offset_width = offset_height = cfg.window_border;
		extra_width = wr.right - wr.left - cr.right + cr.left
				+ offset_width * 2;
		extra_height = wr.bottom - wr.top - cr.bottom + cr.top
				+ offset_height * 2;

		if (win_width != font_width * term->cols + offset_width * 2
				|| win_height != font_height * term->rows + offset_height * 2) {

			static RECT ss;
			int width, height;

			get_fullscreen_rect(&ss);

			width = (ss.right - ss.left - extra_width) / font_width;
			height = (ss.bottom - ss.top - extra_height) / font_height;

			/* Grrr too big */
			if (term->rows > height || term->cols > width) {
				if (cfg.resize_action == RESIZE_EITHER) {
					/* Make the font the biggest we can */
					if (term->cols > width)
						font_width = (ss.right - ss.left - extra_width)
								/ term->cols;
					if (term->rows > height)
						font_height = (ss.bottom - ss.top - extra_height)
								/ term->rows;

					deinit_fonts();
					init_fonts(font_width, font_height);

					width = (ss.right - ss.left - extra_width) / font_width;
					height = (ss.bottom - ss.top - extra_height) / font_height;
				} else {
					if (height > term->rows)
						height = term->rows;
					if (width > term->cols)
						width = term->cols;
					term_size(term, height, width, cfg.savelines);
#ifdef RDB_DEBUG_PATCH
					debug((27, "reset_window() -> term resize to (%d,%d)",
									height, width));
#endif
				}
			}

			SetWindowPos(hwnd, NULL, 0, 0,
					font_width * term->cols + extra_width,
					font_height * term->rows + extra_height,
					SWP_NOMOVE | SWP_NOZORDER);

			InvalidateRect(hwnd, NULL, TRUE);
#ifdef RDB_DEBUG_PATCH
			debug((27, "reset_window() -> window resize to (%d,%d)",
							font_width*term->cols + extra_width,
							font_height*term->rows + extra_height));
#endif
		}
		return;
	}

	/* We're allowed to or must change the font but do we want to ?  */

	if (font_width != (win_width - cfg.window_border * 2) / term->cols
			|| font_height
					!= (win_height - cfg.window_border * 2) / term->rows) {

		deinit_fonts();
		init_fonts((win_width - cfg.window_border * 2) / term->cols,
				(win_height - cfg.window_border * 2) / term->rows);
		offset_width = (win_width - font_width * term->cols) / 2;
		offset_height = (win_height - font_height * term->rows) / 2;

		extra_width = wr.right - wr.left - cr.right + cr.left
				+ offset_width * 2;
		extra_height = wr.bottom - wr.top - cr.bottom + cr.top
				+ offset_height * 2;

		InvalidateRect(hwnd, NULL, TRUE);
#ifdef RDB_DEBUG_PATCH
		debug((25, "reset_window() -> font resize to (%d,%d)",
						font_width, font_height));
#endif
	}
}

void request_paste(void *frontend) {
	/*
	 * In Windows, pasting is synchronous: we can read the
	 * clipboard with no difficulty, so request_paste() can just go
	 * ahead and paste.
	 */
	term_do_paste(term);
}

void set_title(void *frontend, char *title) {
	sfree(window_name);
	window_name = snewn(1 + strlen(title), char);
	strcpy(window_name, title);
	if (cfg.win_name_always || !IsIconic(hwnd))
		SetWindowText(hwnd, title);
}

void set_icon(void *frontend, char *title) {
	sfree(icon_name);
	icon_name = snewn(1 + strlen(title), char);
	strcpy(icon_name, title);
	if (!cfg.win_name_always && IsIconic(hwnd))
		SetWindowText(hwnd, title);
}

void set_sbar(void *frontend, int total, int start, int page) {
	SCROLLINFO si;

	if (is_full_screen() ? !cfg.scrollbar_in_fullscreen : !cfg.scrollbar)
		return;

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = total - 1;
	si.nPage = page;
	si.nPos = start;
	if (hwnd)
		SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

Context get_ctx(void *frontend) {
	HDC hdc;
	if (hwnd) {
		hdc = GetDC(hwnd);
		if (hdc && pal)
			SelectPalette(hdc, pal, FALSE);
		return hdc;
	} else
		return NULL;
}

void free_ctx(Context ctx) {
	SelectPalette(ctx, GetStockObject(DEFAULT_PALETTE), FALSE);
	ReleaseDC(hwnd, ctx);
}

static void real_palette_set(int n, int r, int g, int b) {
	if (pal) {
		logpal->palPalEntry[n].peRed = r;
		logpal->palPalEntry[n].peGreen = g;
		logpal->palPalEntry[n].peBlue = b;
		logpal->palPalEntry[n].peFlags = PC_NOCOLLAPSE;
		colours[n] = PALETTERGB(r, g, b);
		SetPaletteEntries(pal, 0, NALLCOLOURS, logpal->palPalEntry);
	} else
		colours[n] = RGB(r, g, b);
}

void palette_set(void *frontend, int n, int r, int g, int b) {
	if (n >= 16)
		n += 256 - 16;
	if (n > NALLCOLOURS)
		return;
	real_palette_set(n, r, g, b);
	if (pal) {
		HDC hdc = get_ctx(frontend);
		UnrealizeObject(pal);
		RealizePalette(hdc);
		free_ctx(hdc);
	} else {
		if (n == (ATTR_DEFBG >> ATTR_BGSHIFT))
			/* If Default Background changes, we need to ensure any
			 * space between the text area and the window border is
			 * redrawn. */
			InvalidateRect(hwnd, NULL, TRUE);
	}
}

void palette_reset(void *frontend) {
	int i;

	/* And this */
	for (i = 0; i < NALLCOLOURS; i++) {
		if (pal) {
			logpal->palPalEntry[i].peRed = defpal[i].rgbtRed;
			logpal->palPalEntry[i].peGreen = defpal[i].rgbtGreen;
			logpal->palPalEntry[i].peBlue = defpal[i].rgbtBlue;
			logpal->palPalEntry[i].peFlags = 0;
			colours[i] = PALETTERGB(defpal[i].rgbtRed,
					defpal[i].rgbtGreen,
					defpal[i].rgbtBlue);
		} else
			colours[i] = RGB(defpal[i].rgbtRed,
					defpal[i].rgbtGreen, defpal[i].rgbtBlue);
	}

	if (pal) {
		HDC hdc;
		SetPaletteEntries(pal, 0, NALLCOLOURS, logpal->palPalEntry);
		hdc = get_ctx(frontend);
		RealizePalette(hdc);
		free_ctx(hdc);
	} else {
		/* Default Background may have changed. Ensure any space between
		 * text area and window border is redrawn. */
		InvalidateRect(hwnd, NULL, TRUE);
	}
}

void write_aclip(void *frontend, char *data, int len, int must_deselect) {
	HGLOBAL clipdata;
	void *lock;

	clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len + 1);
	if (!clipdata)
		return;
	lock = GlobalLock(clipdata);
	if (!lock)
		return;
	memcpy(lock, data, len);
	((unsigned char *) lock)[len] = 0;
	GlobalUnlock(clipdata);

	if (!must_deselect)
		SendMessage(hwnd, WM_IGNORE_CLIP, TRUE, 0);

	if (OpenClipboard(hwnd)) {
		EmptyClipboard();
		SetClipboardData(CF_TEXT, clipdata);
		CloseClipboard();
	} else
		GlobalFree(clipdata);

	if (!must_deselect)
		SendMessage(hwnd, WM_IGNORE_CLIP, FALSE, 0);
}

/*
 * Note: unlike write_aclip() this will not append a nul.
 */
void write_clip(void *frontend, wchar_t * data, int *attr, int len,
		int must_deselect) {
	HGLOBAL clipdata, clipdata2, clipdata3;
	int len2;
	void *lock, *lock2, *lock3;

	len2 = WideCharToMultiByte(CP_ACP, 0, data, len, 0, 0, NULL, NULL);

	clipdata = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
			len * sizeof(wchar_t));
	clipdata2 = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, len2);

	if (!clipdata || !clipdata2) {
		if (clipdata)
			GlobalFree(clipdata);
		if (clipdata2)
			GlobalFree(clipdata2);
		return;
	}
	if (!(lock = GlobalLock(clipdata)))
		return;
	if (!(lock2 = GlobalLock(clipdata2)))
		return;

	memcpy(lock, data, len * sizeof(wchar_t));
	WideCharToMultiByte(CP_ACP, 0, data, len, lock2, len2, NULL, NULL);

	if (cfg.rtf_paste) {
		wchar_t unitab[256];
		char *rtf = NULL;
		unsigned char *tdata = (unsigned char *) lock2;
		wchar_t *udata = (wchar_t *) lock;
		int rtflen = 0, uindex = 0, tindex = 0;
		int rtfsize = 0;
		int multilen, blen, alen, totallen, i;
		char before[16], after[4];
		int fgcolour, lastfgcolour = 0;
		int bgcolour, lastbgcolour = 0;
		int attrBold, lastAttrBold = 0;
		int attrUnder, lastAttrUnder = 0;
		int palette[NALLCOLOURS];
		int numcolours;

		get_unitab(CP_ACP, unitab, 0);

		rtfsize = 100 + strlen(cfg.font.name);
		rtf = snewn(rtfsize, char);
		rtflen = sprintf(rtf,
				"{\\rtf1\\ansi\\deff0{\\fonttbl\\f0\\fmodern %s;}\\f0\\fs%d",
				cfg.font.name, cfg.font.height * 2);

		/*
		 * Add colour palette
		 * {\colortbl ;\red255\green0\blue0;\red0\green0\blue128;}
		 */

		/*
		 * First - Determine all colours in use
		 *    o  Foregound and background colours share the same palette
		 */
		if (attr) {
			memset(palette, 0, sizeof(palette));
			for (i = 0; i < (len - 1); i++) {
				fgcolour = ((attr[i] & ATTR_FGMASK) >> ATTR_FGSHIFT);
				bgcolour = ((attr[i] & ATTR_BGMASK) >> ATTR_BGSHIFT);

				if (attr[i] & ATTR_REVERSE) {
					int tmpcolour = fgcolour; /* Swap foreground and background */
					fgcolour = bgcolour;
					bgcolour = tmpcolour;
				}

				if (bold_mode == BOLD_COLOURS && (attr[i] & ATTR_BOLD)) {
					if (fgcolour < 8) /* ANSI colours */
						fgcolour += 8;
					else if (fgcolour >= 256) /* Default colours */
						fgcolour++;
				}

				if (attr[i] & ATTR_BLINK) {
					if (bgcolour < 8) /* ANSI colours */
						bgcolour += 8;
					else if (bgcolour >= 256) /* Default colours */
						bgcolour++;
				}

				palette[fgcolour]++;
				palette[bgcolour]++;
			}

			/*
			 * Next - Create a reduced palette
			 */
			numcolours = 0;
			for (i = 0; i < NALLCOLOURS; i++) {
				if (palette[i] != 0)
					palette[i] = ++numcolours;
			}

			/*
			 * Finally - Write the colour table
			 */
			rtf = sresize(rtf, rtfsize + (numcolours * 25), char);
			strcat(rtf, "{\\colortbl ;");
			rtflen = strlen(rtf);

			for (i = 0; i < NALLCOLOURS; i++) {
				if (palette[i] != 0) {
					rtflen += sprintf(&rtf[rtflen], "\\red%d\\green%d\\blue%d;",
							defpal[i].rgbtRed, defpal[i].rgbtGreen,
							defpal[i].rgbtBlue);
				}
			}
			strcpy(&rtf[rtflen], "}");
			rtflen++;
		}

		/*
		 * We want to construct a piece of RTF that specifies the
		 * same Unicode text. To do this we will read back in
		 * parallel from the Unicode data in `udata' and the
		 * non-Unicode data in `tdata'. For each character in
		 * `tdata' which becomes the right thing in `udata' when
		 * looked up in `unitab', we just copy straight over from
		 * tdata. For each one that doesn't, we must WCToMB it
		 * individually and produce a \u escape sequence.
		 *
		 * It would probably be more robust to just bite the bullet
		 * and WCToMB each individual Unicode character one by one,
		 * then MBToWC each one back to see if it was an accurate
		 * translation; but that strikes me as a horrifying number
		 * of Windows API calls so I want to see if this faster way
		 * will work. If it screws up badly we can always revert to
		 * the simple and slow way.
		 */
		while (tindex < len2 && uindex < len && tdata[tindex] && udata[uindex]) {
			if (tindex + 1 < len2 && tdata[tindex] == '\r'
					&& tdata[tindex + 1] == '\n') {
				tindex++;
				uindex++;
			}

			/*
			 * Set text attributes
			 */
			if (attr) {
				if (rtfsize < rtflen + 64) {
					rtfsize = rtflen + 512;
					rtf = sresize(rtf, rtfsize, char);
				}

				/*
				 * Determine foreground and background colours
				 */
				fgcolour = ((attr[tindex] & ATTR_FGMASK) >> ATTR_FGSHIFT);
				bgcolour = ((attr[tindex] & ATTR_BGMASK) >> ATTR_BGSHIFT);

				if (attr[tindex] & ATTR_REVERSE) {
					int tmpcolour = fgcolour; /* Swap foreground and background */
					fgcolour = bgcolour;
					bgcolour = tmpcolour;
				}

				if (bold_mode == BOLD_COLOURS && (attr[tindex] & ATTR_BOLD)) {
					if (fgcolour < 8) /* ANSI colours */
						fgcolour += 8;
					else if (fgcolour >= 256) /* Default colours */
						fgcolour++;
				}

				if (attr[tindex] & ATTR_BLINK) {
					if (bgcolour < 8) /* ANSI colours */
						bgcolour += 8;
					else if (bgcolour >= 256) /* Default colours */
						bgcolour++;
				}

				/*
				 * Collect other attributes
				 */
				if (bold_mode != BOLD_COLOURS)
					attrBold = attr[tindex] & ATTR_BOLD;
				else
					attrBold = 0;

				attrUnder = attr[tindex] & ATTR_UNDER;

				/*
				 * Reverse video
				 *   o  If video isn't reversed, ignore colour attributes for default foregound
				 *	or background.
				 *   o  Special case where bolded text is displayed using the default foregound
				 *      and background colours - force to bolded RTF.
				 */
				if (!(attr[tindex] & ATTR_REVERSE)) {
					if (bgcolour >= 256) /* Default color */
						bgcolour = -1; /* No coloring */

					if (fgcolour >= 256) { /* Default colour */
						if (bold_mode == BOLD_COLOURS && (fgcolour & 1)
								&& bgcolour == -1)
							attrBold = ATTR_BOLD; /* Emphasize text with bold attribute */

						fgcolour = -1; /* No coloring */
					}
				}

				/*
				 * Write RTF text attributes
				 */
				if (lastfgcolour != fgcolour) {
					lastfgcolour = fgcolour;
					rtflen += sprintf(&rtf[rtflen], "\\cf%d ",
							(fgcolour >= 0) ? palette[fgcolour] : 0);
				}

				if (lastbgcolour != bgcolour) {
					lastbgcolour = bgcolour;
					rtflen += sprintf(&rtf[rtflen], "\\highlight%d ",
							(bgcolour >= 0) ? palette[bgcolour] : 0);
				}

				if (lastAttrBold != attrBold) {
					lastAttrBold = attrBold;
					rtflen += sprintf(&rtf[rtflen], "%s",
							attrBold ? "\\b " : "\\b0 ");
				}

				if (lastAttrUnder != attrUnder) {
					lastAttrUnder = attrUnder;
					rtflen += sprintf(&rtf[rtflen], "%s",
							attrUnder ? "\\ul " : "\\ulnone ");
				}
			}

			if (unitab[tdata[tindex]] == udata[uindex]) {
				multilen = 1;
				before[0] = '\0';
				after[0] = '\0';
				blen = alen = 0;
			} else {
				multilen = WideCharToMultiByte(CP_ACP, 0, unitab + uindex, 1,
						NULL, 0, NULL, NULL);
				if (multilen != 1) {
					blen = sprintf(before, "{\\uc%d\\u%d", multilen,
							udata[uindex]);
					alen = 1;
					strcpy(after, "}");
				} else {
					blen = sprintf(before, "\\u%d", udata[uindex]);
					alen = 0;
					after[0] = '\0';
				}
			}
			if (!(tindex + multilen <= len2)) {
				return;
			}
			//assert(tindex + multilen <= len2);
			totallen = blen + alen;
			for (i = 0; i < multilen; i++) {
				if (tdata[tindex + i] == '\\' || tdata[tindex + i] == '{'
						|| tdata[tindex + i] == '}')
					totallen += 2;
				else if (tdata[tindex + i] == 0x0D || tdata[tindex + i] == 0x0A)
					totallen += 6; /* \par\r\n */
				else if (tdata[tindex + i] > 0x7E || tdata[tindex + i] < 0x20)
					totallen += 4;
				else
					totallen++;
			}

			if (rtfsize < rtflen + totallen + 3) {
				rtfsize = rtflen + totallen + 512;
				rtf = sresize(rtf, rtfsize, char);
			}

			strcpy(rtf + rtflen, before);
			rtflen += blen;
			for (i = 0; i < multilen; i++) {
				if (tdata[tindex + i] == '\\' || tdata[tindex + i] == '{'
						|| tdata[tindex + i] == '}') {
					rtf[rtflen++] = '\\';
					rtf[rtflen++] = tdata[tindex + i];
				} else if (tdata[tindex + i] == 0x0D
						|| tdata[tindex + i] == 0x0A) {
					rtflen += sprintf(rtf + rtflen, "\\par\r\n");
				} else if (tdata[tindex + i] > 0x7E
						|| tdata[tindex + i] < 0x20) {
					rtflen += sprintf(rtf + rtflen, "\\'%02x",
							tdata[tindex + i]);
				} else {
					rtf[rtflen++] = tdata[tindex + i];
				}
			}
			strcpy(rtf + rtflen, after);
			rtflen += alen;

			tindex += multilen;
			uindex++;
		}

		rtf[rtflen++] = '}'; /* Terminate RTF stream */
		rtf[rtflen++] = '\0';
		rtf[rtflen++] = '\0';

		clipdata3 = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, rtflen);
		if (clipdata3 && (lock3 = GlobalLock(clipdata3)) != NULL) {
			memcpy(lock3, rtf, rtflen);
			GlobalUnlock(clipdata3);
		}
		sfree(rtf);
	} else
		clipdata3 = NULL;

	GlobalUnlock(clipdata);
	GlobalUnlock(clipdata2);

	if (!must_deselect)
		SendMessage(hwnd, WM_IGNORE_CLIP, TRUE, 0);

	if (OpenClipboard(hwnd)) {
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, clipdata);
		SetClipboardData(CF_TEXT, clipdata2);
		if (clipdata3)
			SetClipboardData(RegisterClipboardFormat(CF_RTF), clipdata3);
		CloseClipboard();
	} else {
		GlobalFree(clipdata);
		GlobalFree(clipdata2);
	}

	if (!must_deselect)
		SendMessage(hwnd, WM_IGNORE_CLIP, FALSE, 0);
}

void get_clip(void *frontend, wchar_t ** p, int *len) {
	static HGLOBAL clipdata = NULL;
	static wchar_t *converted = 0;
	wchar_t *p2;

	if (converted) {
		sfree(converted);
		converted = 0;
	}
	if (!p) {
		if (clipdata)
			GlobalUnlock(clipdata);
		clipdata = NULL;
		return;
	} else if (OpenClipboard(NULL)) {
		if ((clipdata = GetClipboardData(CF_UNICODETEXT))) {
			CloseClipboard();
			*p = GlobalLock(clipdata);
			if (*p) {
				for (p2 = *p; *p2; p2++)
					;
				*len = p2 - *p;
				return;
			}
		} else if ((clipdata = GetClipboardData(CF_TEXT))) {
			char *s;
			int i;
			CloseClipboard();
			s = GlobalLock(clipdata);
			i = MultiByteToWideChar(CP_ACP, 0, s, strlen(s) + 1, 0, 0);
			*p = converted = snewn(i, wchar_t);
			MultiByteToWideChar(CP_ACP, 0, s, strlen(s) + 1, converted, i);
			*len = i - 1;
			return;
		} else
			CloseClipboard();
	}

	*p = NULL;
	*len = 0;
}

#if 0
/*
 * Move `lines' lines from position `from' to position `to' in the
 * window.
 */
void optimised_move(void *frontend, int to, int from, int lines)
{
	RECT r;
	int min, max;

	min = (to < from ? to : from);
	max = to + from - min;

	r.left = offset_width;
	r.right = offset_width + term->cols * font_width;
	r.top = offset_height + min * font_height;
	r.bottom = offset_height + (max + lines) * font_height;
	ScrollWindow(hwnd, 0, (to - from) * font_height, &r, &r);
}
#endif

static void another_font(int fontno) {
	int basefont;
	int fw_dontcare, fw_bold;
	int c, u, w, x;
	char *s;

	if (fontno < 0 || fontno >= FONT_MAXNO || fontflag[fontno])
		return;

	basefont = (fontno & ~(FONT_BOLDUND));
	if (basefont != fontno && !fontflag[basefont])
		another_font(basefont);

	if (cfg.font.isbold) {
		fw_dontcare = FW_BOLD;
		fw_bold = FW_HEAVY;
	} else {
		fw_dontcare = FW_DONTCARE;
		fw_bold = FW_BOLD;
	}

	c = cfg.font.charset;
	w = fw_dontcare;
	u = FALSE;
	s = cfg.font.name;
	x = font_width;

	if (fontno & FONT_WIDE)
		x *= 2;
	if (fontno & FONT_NARROW)
		x = (x + 1) / 2;
	if (fontno & FONT_OEM)
		c = OEM_CHARSET;
	if (fontno & FONT_BOLD)
		w = fw_bold;
	if (fontno & FONT_UNDERLINE)
		u = TRUE;

	fonts[fontno] = CreateFont(font_height * (1 + !!(fontno & FONT_HIGH)), x, 0,
			0, w, FALSE, u, FALSE, c, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			FONT_QUALITY(cfg.font_quality), FIXED_PITCH | FF_DONTCARE, s);

	fontflag[fontno] = 1;
}

/*
 * Set up, or shut down, an AsyncSelect. Called from winnet.c.
 */
char *do_select(SOCKET skt, int startup) {

	int msg, events;

	if (startup) {
		msg = WM_NETEVENT;
		events = (FD_CONNECT | FD_READ | FD_WRITE | FD_OOB | FD_CLOSE
				| FD_ACCEPT);
	} else {
		msg = events = 0;
	}

	if (!hwnd)
		return "do_select(): internal error (hwnd==NULL)";
	if (p_WSAAsyncSelect(skt, hwnd, msg, events) == SOCKET_ERROR) {
		switch (p_WSAGetLastError()) {
		case WSAENETDOWN:
			return "Network is down";
		default:
			return "WSAAsyncSelect(): unknown error";
		}
	}
	return NULL;
}

/*
 * This is a wrapper to ExtTextOut() to force Windows to display
 * the precise glyphs we give it. Otherwise it would do its own
 * bidi and Arabic shaping, and we would end up uncertain which
 * characters it had put where.
 */
static void exact_textout(HDC hdc, int x, int y, CONST RECT *lprc,
		unsigned short *lpString, UINT cbCount, CONST INT *lpDx, int opaque) {
#ifdef __LCC__
	/*
	 * The LCC include files apparently don't supply the
	 * GCP_RESULTSW type, but we can make do with GCP_RESULTS
	 * proper: the differences aren't important to us (the only
	 * variable-width string parameter is one we don't use anyway).
	 */
	GCP_RESULTS gcpr;
#else
	GCP_RESULTSW gcpr;
#endif
	char *buffer = snewn(cbCount*2+2, char);
	char *classbuffer = snewn(cbCount, char);
	memset(&gcpr, 0, sizeof(gcpr));
	memset(buffer, 0, cbCount * 2 + 2);
	memset(classbuffer, GCPCLASS_NEUTRAL, cbCount);

	gcpr.lStructSize = sizeof(gcpr);
	gcpr.lpGlyphs = (void *) buffer;
	gcpr.lpClass = (void *) classbuffer;
	gcpr.nGlyphs = cbCount;

	GetCharacterPlacementW(hdc, lpString, cbCount, 0, &gcpr,
			FLI_MASK | GCP_CLASSIN | GCP_DIACRITIC);

	ExtTextOut(hdc, x, y,
			ETO_GLYPH_INDEX | ETO_CLIPPED | (opaque ? ETO_OPAQUE : 0), lprc,
			buffer, cbCount, lpDx);
}

/*
 * The exact_textout() wrapper, unfortunately, destroys the useful
 * Windows `font linking' behaviour: automatic handling of Unicode
 * code points not supported in this font by falling back to a font
 * which does contain them. Therefore, we adopt a multi-layered
 * approach: for any potentially-bidi text, we use exact_textout(),
 * and for everything else we use a simple ExtTextOut as we did
 * before exact_textout() was introduced.
 */
static void general_textout(HDC hdc, int x, int y, CONST RECT *lprc,
		unsigned short *lpString, UINT cbCount, CONST INT *lpDx, int opaque) {
	int i, j, xp, xn;
	RECT newrc;

#ifdef FIXME_REMOVE_BEFORE_CHECKIN
	int k;
	debug(("general_textout: %d,%d", x, y));
	for(k=0;k<cbCount;k++)debug((" U+%04X", lpString[k]));
	debug(("\n           rect: [%d,%d %d,%d]", lprc->left, lprc->top, lprc->right, lprc->bottom));
	debug(("\n"));
#endif

	xp = xn = x;

	for (i = 0; i < (int) cbCount;) {
		int rtl = is_rtl(lpString[i]);

		xn += lpDx[i];

		for (j = i + 1; j < (int) cbCount; j++) {
			if (rtl != is_rtl(lpString[j]))
				break;
			xn += lpDx[j];
		}

		/*
		 * Now [i,j) indicates a maximal substring of lpString
		 * which should be displayed using the same textout
		 * function.
		 */
		if (rtl) {
			newrc.left = lprc->left + xp - x;
			newrc.right = lprc->left + xn - x;
			newrc.top = lprc->top;
			newrc.bottom = lprc->bottom;
#ifdef FIXME_REMOVE_BEFORE_CHECKIN
			{
				int k;
				debug(("  exact_textout: %d,%d", xp, y));
				for(k=0;k<j-i;k++)debug((" U+%04X", lpString[i+k]));
				debug(("\n           rect: [%d,%d %d,%d]\n", newrc.left, newrc.top, newrc.right, newrc.bottom));
			}
#endif
			exact_textout(hdc, xp, y, &newrc, lpString + i, j - i, lpDx + i,
					opaque);
		} else {
#ifdef FIXME_REMOVE_BEFORE_CHECKIN
			{
				int k;
				debug(("  ExtTextOut   : %d,%d", xp, y));
				for(k=0;k<j-i;k++)debug((" U+%04X", lpString[i+k]));
				debug(("\n           rect: [%d,%d %d,%d]\n", newrc.left, newrc.top, newrc.right, newrc.bottom));
			}
#endif
			newrc.left = lprc->left + xp - x;
			newrc.right = lprc->left + xn - x;
			newrc.top = lprc->top;
			newrc.bottom = lprc->bottom;
			ExtTextOutW(hdc, xp, y, ETO_CLIPPED | (opaque ? ETO_OPAQUE : 0),
					&newrc, lpString + i, j - i, lpDx + i);
		}

		i = j;
		xp = xn;
	}

#ifdef FIXME_REMOVE_BEFORE_CHECKIN
	debug(("general_textout: done, xn=%d\n", xn));
#endif
	//assert(xn - x == lprc->right - lprc->left);//如果表达式条件不满足,则报出异常
}

/*
 * Draw a line of text in the window, at given character
 * coordinates, in given attributes.
 *
 * We are allowed to fiddle with the contents of `text'.
 */
void do_text_internal(Context ctx, int x, int y, wchar_t *text, int len,
		unsigned long attr, int lattr) {

	COLORREF fg, bg, t;
	int nfg, nbg, nfont;
	HDC hdc = ctx;
	RECT line_box;
	int force_manual_underline = 0;
	int fnt_width, char_width;
	int text_adjust = 0;
	static int *IpDx = 0, IpDxLEN = 0;

	lattr &= LATTR_MODE;

	char_width = fnt_width = font_width * (1 + (lattr != LATTR_NORM));

	if (attr & ATTR_WIDE)
		char_width *= 2;

	if (len > IpDxLEN || IpDx[0] != char_width) {
		int i;
		if (len > IpDxLEN) {
			sfree(IpDx);
			IpDx = snewn(len + 16, int);
			IpDxLEN = (len + 16);
		}
		for (i = 0; i < IpDxLEN; i++)
			IpDx[i] = char_width;
	}

	/* Only want the left half of double width lines */
	if (lattr != LATTR_NORM && x * 2 >= term->cols)
		return;

	x *= fnt_width;
	y *= font_height;
	x += offset_width;
	y += offset_height;

	if ((attr & TATTR_ACTCURS) && (cfg.cursor_type == 0 || term->big_cursor)) {
		attr &= ~(ATTR_REVERSE | ATTR_BLINK | ATTR_COLOURS);
		if (bold_mode == BOLD_COLOURS)
			attr &= ~ATTR_BOLD;

		/* cursor fg and bg */
		attr |= (260 << ATTR_FGSHIFT) | (261 << ATTR_BGSHIFT);
	}

	nfont = 0;
	if (cfg.vtmode == VT_POORMAN && lattr != LATTR_NORM) {
		/* Assume a poorman font is borken in other ways too. */
		lattr = LATTR_WIDE;
	} else
		switch (lattr) {
		case LATTR_NORM:
			break;
		case LATTR_WIDE:
			nfont |= FONT_WIDE;
			break;
		default:
			nfont |= FONT_WIDE + FONT_HIGH;
			break;
		}
	if (attr & ATTR_NARROW)
		nfont |= FONT_NARROW;

	/* Special hack for the VT100 linedraw glyphs. */
	if (text[0] >= 0x23BA && text[0] <= 0x23BD) {
		switch ((unsigned char) (text[0])) {
		case 0xBA:
			text_adjust = -2 * font_height / 5;
			break;
		case 0xBB:
			text_adjust = -1 * font_height / 5;
			break;
		case 0xBC:
			text_adjust = font_height / 5;
			break;
		case 0xBD:
			text_adjust = 2 * font_height / 5;
			break;
		}
		if (lattr == LATTR_TOP || lattr == LATTR_BOT)
			text_adjust *= 2;
		text[0] = ucsdata.unitab_xterm['q'];
		if (attr & ATTR_UNDER) {
			attr &= ~ATTR_UNDER;
			force_manual_underline = 1;
		}
	}
	/* Anything left as an original character set is unprintable. */
	if (DIRECT_CHAR(text[0])) {
		int i;
		for (i = 0; i < len; i++)
			text[i] = 0xFFFD;
	}

	/* OEM CP */
	if ((text[0] & CSET_MASK) == CSET_OEMCP)
		nfont |= FONT_OEM;

	nfg = ((attr & ATTR_FGMASK) >> ATTR_FGSHIFT);
	nbg = ((attr & ATTR_BGMASK) >> ATTR_BGSHIFT);
	if (bold_mode == BOLD_FONT && (attr & ATTR_BOLD))
		nfont |= FONT_BOLD;
	if (und_mode == UND_FONT && (attr & ATTR_UNDER))
		nfont |= FONT_UNDERLINE;
	another_font(nfont);
	if (!fonts[nfont]) {
		if (nfont & FONT_UNDERLINE)
			force_manual_underline = 1;
		/* Don't do the same for manual bold, it could be bad news. */

		nfont &= ~(FONT_BOLD | FONT_UNDERLINE);
	}
	another_font(nfont);
	if (!fonts[nfont])
		nfont = FONT_NORMAL;
	if (attr & ATTR_REVERSE) {
		t = nfg;
		nfg = nbg;
		nbg = t;
	}
	if (bold_mode == BOLD_COLOURS && (attr & ATTR_BOLD)) {
		if (nfg < 16)
			nfg |= 8;
		else if (nfg >= 256)
			nfg |= 1;
	}
	if (bold_mode == BOLD_COLOURS && (attr & ATTR_BLINK)) {
		if (nbg < 16)
			nbg |= 8;
		else if (nbg >= 256)
			nbg |= 1;
	}
	fg = colours[nfg];
	bg = colours[nbg];
	SelectObject(hdc, fonts[nfont]);
	SetTextColor(hdc, fg);
	SetBkColor(hdc, bg);
	if (attr & TATTR_COMBINING)
		SetBkMode(hdc, TRANSPARENT);
	else
		SetBkMode(hdc, OPAQUE);
	line_box.left = x;
	line_box.top = y;
	line_box.right = x + char_width * len;
	line_box.bottom = y + font_height;

	/* Only want the left half of double width lines */
	if (line_box.right > font_width * term->cols + offset_width)
		line_box.right = font_width * term->cols + offset_width;

	/* We're using a private area for direct to font. (512 chars.) */
	if (ucsdata.dbcs_screenfont && (text[0] & CSET_MASK) == CSET_ACP) {
		/* Ho Hum, dbcs fonts are a PITA! */
		/* To display on W9x I have to convert to UCS */
		static wchar_t *uni_buf = 0;
		static int uni_len = 0;

		int nlen, mptr;
		if (len > uni_len) {
			sfree(uni_buf);
			uni_len = len;
			uni_buf = snewn(uni_len, wchar_t);
		}

		for (nlen = mptr = 0; mptr < len; mptr++) {
			uni_buf[nlen] = 0xFFFD;
			if (IsDBCSLeadByteEx(ucsdata.font_codepage, (BYTE) text[mptr])) {
				char dbcstext[3];

				memset(dbcstext, 0x00, 3);
				dbcstext[0] = text[mptr] & 0xFF;
				dbcstext[1] = text[mptr + 1] & 0xFF;
				if (mptr + 1 < len) {
					IpDx[nlen] += char_width;
					MultiByteToWideChar(ucsdata.font_codepage, MB_USEGLYPHCHARS,
							dbcstext, 2, uni_buf + nlen, 1);
				} else {
					//IpDx[nlen] += fnt_width;
					nlen--;
				}
				mptr++;
			} else {
				char dbcstext[1];
				dbcstext[0] = text[mptr] & 0xFF;
				MultiByteToWideChar(ucsdata.font_codepage, MB_USEGLYPHCHARS,
						dbcstext, 1, uni_buf + nlen, 1);
			}
			nlen++;
		}
		if (nlen <= 0) {
			return; /* Eeek! */
		}
		ExtTextOutW(hdc, x,
				y - font_height * (lattr == LATTR_BOT) + text_adjust,
				ETO_CLIPPED | ETO_OPAQUE, &line_box, uni_buf, nlen, IpDx);

		if (bold_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
			SetBkMode(hdc, TRANSPARENT);
			ExtTextOutW(hdc, x - 1,
					y - font_height * (lattr == LATTR_BOT) + text_adjust,
					ETO_CLIPPED, &line_box, uni_buf, nlen, IpDx);
		}

		IpDx[0] = -1;
	} else if (DIRECT_FONT(text[0])) {
		static char *directbuf = NULL;
		static int directlen = 0;
		int i;
		if (len > directlen) {
			directlen = len;
			directbuf = sresize(directbuf, directlen, char);
		}

		for (i = 0; i < len; i++) {
			directbuf[i] = text[i] & 0xFF;
		}

		ExtTextOut(hdc, x, y - font_height * (lattr == LATTR_BOT) + text_adjust,
				ETO_CLIPPED | ETO_OPAQUE, &line_box, directbuf, len, IpDx);
		if (bold_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
			SetBkMode(hdc, TRANSPARENT);

			/* GRR: This draws the character outside it's box and can leave
			 * 'droppings' even with the clip box! I suppose I could loop it
			 * one character at a time ... yuk.
			 *
			 * Or ... I could do a test print with "W", and use +1 or -1 for this
			 * shift depending on if the leftmost column is blank...
			 */
			ExtTextOut(hdc, x - 1,
					y - font_height * (lattr == LATTR_BOT) + text_adjust,
					ETO_CLIPPED, &line_box, directbuf, len, IpDx);
		}
	} else {
		/* And 'normal' unicode characters */
		static WCHAR *wbuf = NULL;
		static int wlen = 0;
		int i;

		if (wlen < len) {
			sfree(wbuf);
			wlen = len;
			wbuf = snewn(wlen, WCHAR);
		}

		for (i = 0; i < len; i++) {
			wbuf[i] = text[i];
		}
		/* print Glyphs as they are, without Windows' Shaping*/
		general_textout(hdc, x,
				y - font_height * (lattr == LATTR_BOT) + text_adjust, &line_box,
				wbuf, len, IpDx, !(attr & TATTR_COMBINING));

		/* And the shadow bold hack. */
		if (bold_mode == BOLD_SHADOW && (attr & ATTR_BOLD)) {
			SetBkMode(hdc, TRANSPARENT);
			ExtTextOutW(hdc, x - 1,
					y - font_height * (lattr == LATTR_BOT) + text_adjust,
					ETO_CLIPPED, &line_box, wbuf, len, IpDx);
		}
	}
	if (lattr != LATTR_TOP
			&& (force_manual_underline
					|| (und_mode == UND_LINE && (attr & ATTR_UNDER)))) {
		HPEN oldpen;
		int dec = descent;
		if (lattr == LATTR_BOT)
			dec = dec * 2 - font_height;

		oldpen = SelectObject(hdc, CreatePen(PS_SOLID, 0, fg));
		MoveToEx(hdc, x, y + dec, NULL);
		LineTo(hdc, x + len * char_width, y + dec);
		oldpen = SelectObject(hdc, oldpen);
		DeleteObject(oldpen);
	}
}

/*
 * Wrapper that handles combining characters.
 */
void do_text(Context ctx, int x, int y, wchar_t *text, int len,
		unsigned long attr, int lattr) {
	if (attr & TATTR_COMBINING) {
		unsigned long a = 0;
		attr &= ~TATTR_COMBINING;
		while (len--) {
			do_text_internal(ctx, x, y, text, 1, attr | a, lattr);
			text++;
			a = TATTR_COMBINING;
		}
	} else {
		do_text_internal(ctx, x, y, text, len, attr, lattr);
	}

}

void do_cursor(Context ctx, int x, int y, wchar_t *text, int len,
		unsigned long attr, int lattr) {

	int fnt_width;
	int char_width;
	HDC hdc = ctx;
	int ctype = cfg.cursor_type;

	lattr &= LATTR_MODE;

	if ((attr & TATTR_ACTCURS) && (ctype == 0 || term->big_cursor)) {
		if (*text != UCSWIDE) {
			do_text(ctx, x, y, text, len, attr, lattr);
			return;
		}
		ctype = 2;
		attr |= TATTR_RIGHTCURS;
	}
	fnt_width = char_width = font_width * (1 + (lattr != LATTR_NORM));
	if (attr & ATTR_WIDE)
		char_width *= 2;
	x *= fnt_width;
	y *= font_height;
	x += offset_width;
	y += offset_height;

	if ((attr & TATTR_PASCURS) && (ctype == 0 || term->big_cursor)) {
		POINT pts[5];
		HPEN oldpen;
		pts[0].x = pts[1].x = pts[4].x = x;
		pts[2].x = pts[3].x = x + char_width - 1;
		pts[0].y = pts[3].y = pts[4].y = y;
		pts[1].y = pts[2].y = y + font_height - 1;
		oldpen = SelectObject(hdc, CreatePen(PS_SOLID, 0, colours[261]));
		Polyline(hdc, pts, 5);
		oldpen = SelectObject(hdc, oldpen);
		DeleteObject(oldpen);
	} else if ((attr & (TATTR_ACTCURS | TATTR_PASCURS)) && ctype != 0) {
		int startx, starty, dx, dy, length, i;
		if (ctype == 1) {
			startx = x;
			starty = y + descent;
			dx = 1;
			dy = 0;
			length = char_width;
		} else {
			int xadjust = 0;
			if (attr & TATTR_RIGHTCURS)
				xadjust = char_width - 1;
			startx = x + xadjust;
			starty = y;
			dx = 0;
			dy = 1;
			length = font_height;
		}
		if (attr & TATTR_ACTCURS) {
			HPEN oldpen;
			oldpen = SelectObject(hdc, CreatePen(PS_SOLID, 0, colours[261]));
			MoveToEx(hdc, startx, starty, NULL);
			LineTo(hdc, startx + dx * length, starty + dy * length);
			oldpen = SelectObject(hdc, oldpen);
			DeleteObject(oldpen);
		} else {
			for (i = 0; i < length; i++) {
				if (i % 2 == 0) {
					SetPixel(hdc, startx, starty, colours[261]);
				}
				startx += dx;
				starty += dy;
			}
		}
	}
}

/* This function gets the actual width of a character in the normal font.
 */
int char_width(Context ctx, int uc) {
	HDC hdc = ctx;
	int ibuf = 0;

	/* If the font max is the same as the font ave width then this
	 * function is a no-op.
	 */
	if (!font_dualwidth)
		return 1;

	switch (uc & CSET_MASK) {
	case CSET_ASCII:
		uc = ucsdata.unitab_line[uc & 0xFF];
		break;
	case CSET_LINEDRW:
		uc = ucsdata.unitab_xterm[uc & 0xFF];
		break;
	case CSET_SCOACS:
		uc = ucsdata.unitab_scoacs[uc & 0xFF];
		break;
	}
	if (DIRECT_FONT(uc)) {
		if (ucsdata.dbcs_screenfont)
			return 1;

		/* Speedup, I know of no font where ascii is the wrong width */
		if ((uc & ~CSET_MASK) >= ' ' && (uc & ~CSET_MASK) <= '~')
			return 1;

		if ((uc & CSET_MASK) == CSET_ACP) {
			SelectObject(hdc, fonts[FONT_NORMAL]);
		} else if ((uc & CSET_MASK) == CSET_OEMCP) {
			another_font(FONT_OEM);
			if (!fonts[FONT_OEM])
				return 0;

			SelectObject(hdc, fonts[FONT_OEM]);
		} else
			return 0;

		if (GetCharWidth32(hdc, uc & ~CSET_MASK, uc & ~CSET_MASK, &ibuf) != 1
				&& GetCharWidth(hdc, uc & ~CSET_MASK, uc & ~CSET_MASK, &ibuf)
						!= 1)
			return 0;
	} else {
		/* Speedup, I know of no font where ascii is the wrong width */
		if (uc >= ' ' && uc <= '~')
			return 1;

		SelectObject(hdc, fonts[FONT_NORMAL]);
		if (GetCharWidth32W(hdc, uc, uc, &ibuf) == 1)
			/* Okay that one worked */
			;
		else if (GetCharWidthW(hdc, uc, uc, &ibuf) == 1)
			/* This should work on 9x too, but it's "less accurate" */
			;
		else
			return 0;
	}

	ibuf += font_width / 2 - 1;
	ibuf /= font_width;

	return ibuf;
}

void request_resize(void *frontend, int w, int h) {
	int width, height;

	/* If the window is maximized supress resizing attempts */
	if (IsZoomed(hwnd)) {
		if (cfg.resize_action == RESIZE_TERM)
			return;
	}

	if (cfg.resize_action == RESIZE_DISABLED)
		return;
	if (h == term->rows && w == term->cols)
		return;

	/* Sanity checks ... */
	{
		static int first_time = 1;
		static RECT ss;

		switch (first_time) {
		case 1:
			/* Get the size of the screen */
			if (get_fullscreen_rect(&ss))
				/* first_time = 0 */;
			else {
				first_time = 2;
				break;
			}
		case 0:
			/* Make sure the values are sane */
			width = (ss.right - ss.left - extra_width) / 4;
			height = (ss.bottom - ss.top - extra_height) / 6;

			if (w > width || h > height)
				return;
			if (w < 15)
				w = 15;
			if (h < 1)
				h = 1;
		}
	}

	term_size(term, h, w, cfg.savelines);

	if (cfg.resize_action != RESIZE_FONT && !IsZoomed(hwnd)) {
		width = extra_width + font_width * w;
		height = extra_height + font_height * h;

		SetWindowPos(hwnd, NULL, 0, 0, width, height,
				SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOZORDER);
	} else
		reset_window(0);

	InvalidateRect(hwnd, NULL, TRUE);
}

void BeforClose(Terminal *term) {
	term_free(term);
	CloseHandle(hMutex);
}

/*
 * Clean up and exit.
 */
void cleanup_exit(int code) {
	/*
	 * Clean up.
	 */
	deinit_fonts();
	sfree(logpal);
	if (pal)
		DeleteObject(pal);
	sk_cleanup();

	if (cfg.protocol == PROT_SSH) {
		random_save_seed();
#ifdef MSCRYPTOAPI
		crypto_wrapup();
#endif
	}
	shutdown_help();
	BeforClose(term);
	exit(code);
}

/*
 * Print a message box and perform a fatal exit.
 */
void fatalbox(char *fmt, ...) {
	va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, fmt);
	stuff = dupvprintf(fmt, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(hwnd, stuff, morestuff, MB_ICONERROR | MB_OK);
	sfree(stuff);
	cleanup_exit(1);
}

/*
 * Print a modal (Really Bad) message box and perform a fatal exit.
 */
void modalfatalbox(char *fmt, ...) {
	va_list ap;
	char *stuff, morestuff[100];

	va_start(ap, fmt);
	stuff = dupvprintf(fmt, ap);
	va_end(ap);
	sprintf(morestuff, "%.70s Fatal Error", appname);
	MessageBox(hwnd, stuff, morestuff, MB_SYSTEMMODAL | MB_ICONERROR | MB_OK);
	sfree(stuff);
	cleanup_exit(1);
}

/*
 * Beep.
 */
void do_beep(void *frontend, int mode) {
	if (mode == BELL_DEFAULT) {
		/*
		 * For MessageBeep style bells, we want to be careful of
		 * timing, because they don't have the nice property of
		 * PlaySound bells that each one cancels the previous
		 * active one. So we limit the rate to one per 50ms or so.
		 */
		static long lastbeep = 0;
		long beepdiff;

		beepdiff = GetTickCount() - lastbeep;
		if (beepdiff >= 0 && beepdiff < 50)
			return;
		MessageBeep(MB_OK);
		/*
		 * The above MessageBeep call takes time, so we record the
		 * time _after_ it finishes rather than before it starts.
		 */
		lastbeep = GetTickCount();
	} else if (mode == BELL_WAVEFILE) {
		if (!PlaySound(cfg.bell_wavefile.path, NULL,
				SND_ASYNC | SND_FILENAME)) {
			char buf[sizeof(cfg.bell_wavefile.path) + 80];
			char otherbuf[100];
			sprintf(buf, "Unable to play sound file\n%s\n"
					"Using default sound instead", cfg.bell_wavefile.path);
			sprintf(otherbuf, "%.70s Sound Error", appname);
			MessageBox(hwnd, buf, otherbuf, MB_OK | MB_ICONEXCLAMATION);
			cfg.beep = BELL_DEFAULT;
			term->cfg.beep = cfg.beep;
		}
	} else if (mode == BELL_PCSPEAKER) {
		static long lastbeep = 0;
		long beepdiff;

		beepdiff = GetTickCount() - lastbeep;
		if (beepdiff >= 0 && beepdiff < 50)
			return;

		/*
		 * We must beep in different ways depending on whether this
		 * is a 95-series or NT-series OS.
		 */
		if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT)
			Beep(800, 100);
		else
			MessageBeep(-1);
		lastbeep = GetTickCount();
	}
	/* Otherwise, either visual bell or disabled; do nothing here */
	if (!term->has_focus) {
		flash_window(2); /* start */
	}
}

/*
 * Minimise or restore the window in response to a server-side
 * request.
 */
void set_iconic(void *frontend, int iconic) {
	if (IsIconic(hwnd)) {
		if (!iconic)
			ShowWindow(hwnd, SW_RESTORE);
	} else {
		if (iconic)
			ShowWindow(hwnd, SW_MINIMIZE);
	}
}

/*
 * Move the window in response to a server-side request.
 */
void move_window(void *frontend, int x, int y) {
	if (cfg.resize_action == RESIZE_DISABLED || cfg.resize_action == RESIZE_FONT
			|| IsZoomed(hwnd))
		return;

	SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/*
 * Move the window to the top or bottom of the z-order in response
 * to a server-side request.
 */
void set_zorder(void *frontend, int top) {
	if (cfg.alwaysontop)
		return; /* ignore */
	SetWindowPos(hwnd, top ? HWND_TOP : HWND_BOTTOM, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE);
}

/*
 * Refresh the window in response to a server-side request.
 */
void refresh_window(void *frontend) {
	InvalidateRect(hwnd, NULL, TRUE);
}

/*
 * Maximise or restore the window in response to a server-side
 * request.
 */
void set_zoomed(void *frontend, int zoomed) {
	if (IsZoomed(hwnd)) {
		if (!zoomed)
			ShowWindow(hwnd, SW_RESTORE);
	} else {
		if (zoomed)
			ShowWindow(hwnd, SW_MAXIMIZE);
	}
}

/*
 * Report whether the window is iconic, for terminal reports.
 */
int is_iconic(void *frontend) {
	return IsIconic(hwnd);
}

/*
 * Report the window's position, for terminal reports.
 */
void get_window_pos(void *frontend, int *x, int *y) {
	RECT r;
	GetWindowRect(hwnd, &r);
	*x = r.left;
	*y = r.top;
}

/*
 * Report the window's pixel size, for terminal reports.
 */
void get_window_pixels(void *frontend, int *x, int *y) {
	RECT r;
	GetWindowRect(hwnd, &r);
	*x = r.right - r.left;
	*y = r.bottom - r.top;
}

/*
 * Return the window or icon title.
 */
char *get_window_title(void *frontend, int icon) {
	return icon ? icon_name : window_name;
}

/*
 * See if we're in full-screen mode.
 */
int is_full_screen() {
	if (!IsZoomed(hwnd))
		return FALSE;
	if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CAPTION)
		return FALSE;
	return TRUE;
}

/* Get the rect/size of a full screen window using the nearest available
 * monitor in multimon systems; default to something sensible if only
 * one monitor is present. */
int get_fullscreen_rect(RECT * ss) {
#if defined(MONITOR_DEFAULTTONEAREST) && !defined(NO_MULTIMON)
	HMONITOR mon;
	MONITORINFO mi;
	mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(mon, &mi);

	/* structure copy */
	*ss = mi.rcMonitor;
	return TRUE;
#else
	/* could also use code like this:
	 ss->left = ss->top = 0;
	 ss->right = GetSystemMetrics(SM_CXSCREEN);
	 ss->bottom = GetSystemMetrics(SM_CYSCREEN);
	 */
	return GetClientRect(GetDesktopWindow(), ss);
#endif
}

/*
 * Go full-screen. This should only be called when we are already
 * maximised.
 */
void make_full_screen() {
	DWORD style;
	RECT ss;

	if (!IsZoomed(hwnd)) {
		return;
	}

	if (is_full_screen())
		return;

	/* Remove the window furniture. */
	style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style &= ~(WS_CAPTION | WS_BORDER | WS_THICKFRAME);
	if (cfg.scrollbar_in_fullscreen)
		style |= WS_VSCROLL;
	else
		style &= ~WS_VSCROLL;
	SetWindowLongPtr(hwnd, GWL_STYLE, style);

	/* Resize ourselves to exactly cover the nearest monitor. */
	get_fullscreen_rect(&ss);
	SetWindowPos(hwnd, HWND_TOP, ss.left, ss.top, ss.right - ss.left,
			ss.bottom - ss.top, SWP_FRAMECHANGED);

	/* We may have changed size as a result */

	reset_window(0);

	/* Tick the menu item in the System menu. */
	CheckMenuItem(GetSystemMenu(hwnd, FALSE), IDM_FULLSCREEN, MF_CHECKED);
}

/*
 * Clear the full-screen attributes.
 */
void clear_full_screen() {
	DWORD oldstyle, style;

	/* Reinstate the window furniture. */
	style = oldstyle = GetWindowLongPtr(hwnd, GWL_STYLE);
	style |= WS_CAPTION | WS_BORDER;
	if (cfg.resize_action == RESIZE_DISABLED)
		style &= ~WS_THICKFRAME;
	else
		style |= WS_THICKFRAME;
	if (cfg.scrollbar)
		style |= WS_VSCROLL;
	else
		style &= ~WS_VSCROLL;
	if (style != oldstyle) {
		SetWindowLongPtr(hwnd, GWL_STYLE, style);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	/* Untick the menu item in the System menu. */
	CheckMenuItem(GetSystemMenu(hwnd, FALSE), IDM_FULLSCREEN, MF_UNCHECKED);
}

/*
 * Toggle full-screen mode.
 */
void flip_full_screen() {
	if (is_full_screen()) {
		ShowWindow(hwnd, SW_RESTORE);
	} else if (IsZoomed(hwnd)) {
		make_full_screen();
	} else {
		SendMessage(hwnd, WM_FULLSCR_ON_MAX, 0, 0);
		ShowWindow(hwnd, SW_MAXIMIZE);
	}
}

void frontend_keypress(void *handle) {
	/*
	 * Keypress termination in non-Close-On-Exit mode is not
	 * currently supported in PuTTY proper, because the window
	 * always has a perfectly good Close button anyway. So we do
	 * nothing here.
	 */
	return;
}

int from_backend(void *frontend, int is_stderr, const char *data, int len) {
	return term_data(term, is_stderr, data, len);
}

int from_backend_untrusted(void *frontend, const char *data, int len) {
	return term_data_untrusted(term, data, len);
}

//int get_userpass_input(prompts_t *p, unsigned char *in, int inlen) {
//	int ret;
//	ret = cmdline_get_passwd_input(p, in, inlen);
//	if (ret == -1)
//		ret = term_get_userpass_input(term, p, in, inlen);
//	return ret;
//}

void agent_schedule_callback(void (*callback)(void *, void *, int),
		void *callback_ctx, void *data, int len) {
	struct agent_callback *c = snew(struct agent_callback);
	c->callback = callback;
	c->callback_ctx = callback_ctx;
	c->data = data;
	c->len = len;
	PostMessage(hwnd, WM_AGENT_CALLBACK, 0, (LPARAM) c);
}

void systopalette(void) {
	int i;
	static const struct {
		int nIndex;
		int norm;
		int bold;
	} or[] = { { COLOR_WINDOWTEXT, 256, 257 }, /* Default Foreground */
	{ COLOR_WINDOW, 258, 259 }, /* Default Background */
	{ COLOR_HIGHLIGHTTEXT, 260, 260 }, /* Cursor Text */
	{ COLOR_HIGHLIGHT, 261, 261 }, /* Cursor Colour */
	};

	for (i = 0; i < (sizeof(or) / sizeof(or[0])); i++) {
		COLORREF colour = GetSysColor(or[i].nIndex);
		defpal[or[i].norm].rgbtRed = defpal[or[i].bold].rgbtRed =
				GetRValue(colour);
		defpal[or[i].norm].rgbtGreen = defpal[or[i].bold].rgbtGreen =
				GetGValue(colour);
		defpal[or[i].norm].rgbtBlue = defpal[or[i].bold].rgbtBlue =
				GetBValue(colour);
	}
}

