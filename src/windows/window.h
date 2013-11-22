#ifdef __cplusplus
extern "C" {
#endif

#define VAR_BACKEND 1

/* From MSDN: In the WM_SYSCOMMAND message, the four low-order bits of
 * wParam are used by Windows, and should be masked off, so we shouldn't
 * attempt to store information in them. Hence all these identifiers have
 * the low 4 bits clear. Also, identifiers should < 0xF000. */

#define IDM_SHOWLOG   0x0010
#define IDM_NEWSESS   0x0020
#define IDM_DUPSESS   0x0030
#define IDM_RESTART   0x0040
#define IDM_RECONF    0x0050
#define IDM_CLRSB     0x0060
#define IDM_RESET     0x0070
#define IDM_HELP      0x0140
#define IDM_ABOUT     0x0150
#define IDM_SAVEDSESS 0x0160
#define IDM_COPYALL   0x0170
#define IDM_PRINTSCREEN 0x1170

#define IDM_FULLSCREEN	0x0180
//#define IDM_PASTE     0x0190
#define IDM_SPECIALSEP 0x0200

#define IDM_CLMENU    0x03110

#define IDM_SPECIAL_MIN 0x0400
#define IDM_SPECIAL_MAX 0x0800

#define IDM_SAVED_MIN 0x1000
#define IDM_SAVED_MAX 0x5000

#define WM_IGNORE_CLIP (WM_APP + 2)
#define WM_FULLSCR_ON_MAX (WM_APP + 3)
#define WM_AGENT_CALLBACK (WM_APP + 4)

#define TIMING_TIMER_ID 1234
#define IDI_MAINICON   200
#define NCFGCOLOURS 22
#define NEXTCOLOURS 240
#define NALLCOLOURS (NCFGCOLOURS + NEXTCOLOURS)
#define IDM_DUPSESS   0x0030
#define IDM_RESTART   0x0040
#define FONT_MAXNO 	0x2F
#define FONT_SHIFT	5
#define FONT_NORMAL 0
#define FONT_BOLD 1
#define FONT_UNDERLINE 2
#define FONT_BOLDUND 3

#define FONT_WIDE	0x04
#define FONT_HIGH	0x08
#define FONT_NARROW	0x10
#define FONT_OEM 	0x20

#define FONT_MAXNO 	0x2F

#define NCFGCOLOURS 22
#define NEXTCOLOURS 240
#define NALLCOLOURS (NCFGCOLOURS + NEXTCOLOURS)

#define IDM_SAVED_MIN 0x1000
#define IDM_SAVED_MAX 0x5000
#define MENU_SAVED_STEP 16
/* Maximum number of sessions on saved-session submenu */
#define MENU_SAVED_MAX ((IDM_SAVED_MAX-IDM_SAVED_MIN) / MENU_SAVED_STEP)

#define MENU_SAVED_STEP 16
/* Maximum number of sessions on saved-session submenu */
#define MENU_SAVED_MAX ((IDM_SAVED_MAX-IDM_SAVED_MIN) / MENU_SAVED_STEP)
/* Needed for Chinese support and apparently not always defined. */
#ifndef VK_PROCESSKEY
#define VK_PROCESSKEY 0xE5
#endif

/* Mouse wheel support. */
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A	       /* not defined in earlier SDKs */
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

void * get_static_point(int type);
void sys_cursor_update(void);
void make_full_screen();
void clear_full_screen();
void flip_full_screen();
void reset_window(int reinit);
int is_full_screen();
void flash_window(int mode);

struct agent_callback {
	void (*callback)(void *, void *, int);
	void *callback_ctx;
	void *data;
	int len;
};

struct _popup_menus {
	HMENU menu;
};

#ifdef __cplusplus
}
#endif
