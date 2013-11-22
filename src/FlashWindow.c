/*
 * FlashWindow.c
 *
 *  Created on: 2012-9-9
 *      Author: zhangbo
 */
#include "putty.h"
#include <windows.h>
#include <winuser.h>

typedef BOOL (WINAPI *p_FlashWindowEx_t)(PFLASHWINFO);
static p_FlashWindowEx_t p_FlashWindowEx = NULL;

void init_flashwindow(void) {
	HMODULE user32_module = LoadLibrary("USER32.DLL");
	if (user32_module) {
		p_FlashWindowEx = (p_FlashWindowEx_t) GetProcAddress(user32_module,
				"FlashWindowEx");
	}
}

static BOOL flash_window_ex(DWORD dwFlags, UINT uCount, DWORD dwTimeout) {
	if (p_FlashWindowEx) {
		FLASHWINFO fi;
		fi.cbSize = sizeof(fi);
		fi.hwnd = hwnd;
		fi.dwFlags = dwFlags;
		fi.uCount = uCount;
		fi.dwTimeout = dwTimeout;
		return (*p_FlashWindowEx)(&fi);
	} else
		return FALSE; /* shrug */
}

void flash_window(int mode);
static long next_flash;
static int flashing = 0;

/*
 * Timer for platforms where we must maintain window flashing manually
 * (e.g., Win95).
 */
static void flash_window_timer(void *ctx, long now) {
	if (flashing && now - next_flash >= 0) {
		flash_window(1);
	}
}

/*
 * Manage window caption / taskbar flashing, if enabled.
 * 0 = stop, 1 = maintain, 2 = start
 */
void flash_window(int mode) {
	if ((mode == 0) || (cfg.beep_ind == B_IND_DISABLED)) {
		/* stop */
		if (flashing) {
			flashing = 0;
			if (p_FlashWindowEx)
				flash_window_ex(FLASHW_STOP, 0, 0);
			else
				FlashWindow(hwnd, FALSE);
		}

	} else if (mode == 2) {
		/* start */
		if (!flashing) {
			flashing = 1;
			if (p_FlashWindowEx) {
				/* For so-called "steady" mode, we use uCount=2, which
				 * seems to be the traditional number of flashes used
				 * by user notifications (e.g., by Explorer).
				 * uCount=0 appears to enable continuous flashing, per
				 * "flashing" mode, although I haven't seen this
				 * documented. */
				flash_window_ex(FLASHW_ALL | FLASHW_TIMER,
						(cfg.beep_ind == B_IND_FLASH ? 0 : 2),
						0 /* system cursor blink rate */);
				/* No need to schedule timer */
			} else {
				FlashWindow(hwnd, TRUE);
				next_flash = schedule_timer(450, flash_window_timer, hwnd);
			}
		}

	} else if ((mode == 1) && (cfg.beep_ind == B_IND_FLASH)) {
		/* maintain */
		if (flashing && !p_FlashWindowEx) {
			FlashWindow(hwnd, TRUE); /* toggle */
			next_flash = schedule_timer(450, flash_window_timer, hwnd);
		}
	}
}
