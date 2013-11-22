/*
 * Printing interface for PuTTY.
 */

#include "../putty.h"
#include <windows.h>
#include <winspool.h>
#include "../util/windows/print.h"
#include "../windows/message.h"
#include "../terminal.h"
#include "../util/prtsvc.h"

struct printer_enum_tag {
	int nprinters;
	DWORD enum_level;
	union {
		LPPRINTER_INFO_4 i4;
		LPPRINTER_INFO_5 i5;
	} info;
};

struct printer_job_tag {
	HANDLE hprinter;
};

static CRITICAL_SECTION critsec;
static int timer = 0;

static char *printer_add_enum(int param, DWORD level, char *buffer, int offset,
		int *nprinters_ptr) {
	DWORD needed, nprinters;

	buffer = sresize(buffer, offset+512, char);

	/*
	 * Exploratory call to EnumPrinters to determine how much space
	 * we'll need for the output. Discard the return value since it
	 * will almost certainly be a failure due to lack of space.
	 */EnumPrinters(param, NULL, level, buffer + offset, 512, &needed,
			&nprinters);

	if (needed < 512)
		needed = 512;

	buffer = sresize(buffer, offset+needed, char);

	if (EnumPrinters(param, NULL, level, buffer + offset, needed, &needed,
			&nprinters) == 0)
		return NULL;

	*nprinters_ptr += nprinters;

	return buffer;
}

printer_enum *printer_start_enum(int *nprinters_ptr) {
	printer_enum *ret = snew(printer_enum);
	char *buffer = NULL, *retval;

	*nprinters_ptr = 0; /* default return value */
	buffer = snewn(512, char);

	/*
	 * Determine what enumeration level to use.
	 * When enumerating printers, we need to use PRINTER_INFO_4 on
	 * NT-class systems to avoid Windows looking too hard for them and
	 * slowing things down; and we need to avoid PRINTER_INFO_5 as
	 * we've seen network printers not show up.
	 * On 9x-class systems, PRINTER_INFO_4 isn't available and
	 * PRINTER_INFO_5 is recommended.
	 * Bletch.
	 */
	if (osVersion.dwPlatformId != VER_PLATFORM_WIN32_NT) {
		ret->enum_level = 5;
	} else {
		ret->enum_level = 4;
	}

	retval = printer_add_enum(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
			ret->enum_level, buffer, 0, nprinters_ptr);
	if (!retval)
		goto error;
	else
		buffer = retval;

	switch (ret->enum_level) {
	case 4:
		ret->info.i4 = (LPPRINTER_INFO_4) buffer;
		break;
	case 5:
		ret->info.i5 = (LPPRINTER_INFO_5) buffer;
		break;
	}
	ret->nprinters = *nprinters_ptr;

	return ret;

	error:
	sfree(buffer);
	sfree(ret);
	*nprinters_ptr = 0;
	return NULL;
}

char *printer_get_name(printer_enum *pe, int i) {
	if (!pe)
		return NULL;
	if (i < 0 || i >= pe->nprinters)
		return NULL;
	switch (pe->enum_level) {
	case 4:
		return pe->info.i4[i].pPrinterName;
	case 5:
		return pe->info.i5[i].pPrinterName;
	default:
		return NULL;
	}
}

void printer_finish_enum(printer_enum *pe) {
	if (!pe)
		return;
	switch (pe->enum_level) {
	case 4:
		sfree(pe->info.i4);
		break;
	case 5:
		sfree(pe->info.i5);
		break;
	}
	sfree(pe);
}

void printer_flush_data(printer_job *pj) {
	EndPagePrinter(pj->hprinter);
	EndDocPrinter(pj->hprinter);
	DOC_INFO_1 docinfo;
	docinfo.pDocName = "terminal printer output";
	docinfo.pOutputFile = NULL;
	docinfo.pDatatype = "RAW";
	StartDocPrinter(pj->hprinter, 1, (LPSTR) &docinfo);
	StartPagePrinter(pj->hprinter);
}

static unsigned long WINAPI printer_flush_thread(void* device) {
	printer_job *pj = (printer_job*) device;
	while (1) {
		while (timer > 0) {
			Sleep(200);
			EnterCriticalSection(&critsec);
			if (--timer == 0) {
				printer_flush_data(pj);
			}
			LeaveCriticalSection(&critsec);
		}
		Sleep(100);
	}
	return 0;
}

void printer_fflush_data(printer_job *pj) {
	EndPagePrinter(pj->hprinter);
	EndDocPrinter(pj->hprinter);
	DOC_INFO_1 docinfo;
	docinfo.pDocName = "terminal printer output";
	docinfo.pOutputFile = NULL;
	docinfo.pDatatype = "RAW";
	StartDocPrinter(pj->hprinter, 1, (LPSTR) &docinfo);
	StartPagePrinter(pj->hprinter);
}

static unsigned long WINAPI printer_fflush_thread(void* device) {
	printer_job *pj = (printer_job*) device;
	while (1) {
		while (timer > 0) {
			Sleep(200);
			EnterCriticalSection(&critsec);
			if (--timer == 0) {
				printer_flush_data(pj);
			}
			LeaveCriticalSection(&critsec);
		}
		Sleep(100);
	}
	return 0;
}

printer_job *printer_start_job(char *printer) {
	printer_job *ret = snew(printer_job);
	DOC_INFO_1 docinfo;
	int jobstarted = 0, pagestarted = 0;

	ret->hprinter = NULL;
	if (!OpenPrinter(printer, &ret->hprinter, NULL))
		goto error;

	docinfo.pDocName = "terminal printer output";
	docinfo.pOutputFile = NULL;
	docinfo.pDatatype = "RAW";

	if (!StartDocPrinter(ret->hprinter, 1, (LPSTR) &docinfo))
		goto error;
	jobstarted = 1;

	if (!StartPagePrinter(ret->hprinter))
		goto error;
	pagestarted = 1;

	InitializeCriticalSection(&critsec);
	CreateThread(NULL, 0, printer_flush_thread, ret, 0, 0);

	return ret;

	error: if (pagestarted)
		EndPagePrinter(ret->hprinter);
	if (jobstarted)
		EndDocPrinter(ret->hprinter);
	if (ret->hprinter)
		ClosePrinter(ret->hprinter);
	sfree(ret);
	return NULL;
}

void printer_job_data(printer_job *pj, void *data, int len) {
	DWORD written;

	if (!pj)
		return;

	EnterCriticalSection(&critsec);
	WritePrinter(pj->hprinter, data, len, &written);
	timer = 15;
	LeaveCriticalSection(&critsec);
}

void printer_finish_job(printer_job *pj) {
	if (!pj)
		return;

	DeleteCriticalSection(&critsec);
	EndPagePrinter(pj->hprinter);
	EndDocPrinter(pj->hprinter);
	ClosePrinter(pj->hprinter);
	sfree(pj);
}

static CRITICAL_SECTION critsec;

static void* lpt = NULL;

typedef struct {
	int len;
	void* buf;
} printbuff;

//WINAPI 直接并口打印
static unsigned long printer_dprint_buff(void* data) {
	static char prefix[] = { 0x10, 0x42, 0x1B, 0x36, 0x1B, 0x28, 0x48, 0x00 };
//	static char prefix[] = {0x1B, 0x36, 0x1B, 0x28, 0x48};
	static int prelen = 8;
	static char suffix[] = { 0x0C };
	static int suflen = 1;

	printbuff* pd;
	if (lpt != (void*) 0) {

//EnterCriticalSection(&critsec);

		pd = (printbuff*) data;
		if (lptwrite(lpt, prefix, prelen) == 0) {
			lptwrite(lpt, pd->buf, pd->len);
			lptwrite(lpt, suffix, suflen);
		}

		//printer_finish_job(job);
		sfree(pd->buf);
		sfree(pd);
//  LeaveCriticalSection(&critsec);

		return 0;
	} else {
		MessageBox(NULL, ERROR_INITPRINT, TITLE_POINT, MB_ICONERROR | MB_OK);
		return 1;
	}

}

unsigned long printer_init() {
	static char suffix[] = { 0x0c, 0x1b, 0x62 };
	static int suflen = 3;
	if (lpt != (void*) 0) {
		lptwrite(lpt, suffix, suflen);
		return 0;
	} else
		return 1;

}

//WINAPI 文件打印
static unsigned long printer_fprint_buff(void* data) {
	static char prefix[] = { 0x10, 0x42, 0x1B, 0x36, 0x1B, 0x28, 0x48, 0x00 };
	static int prelen = 8;
	static char suffix[] = { 0x0C };
	static int suflen = 1;
	printbuff* pd;
	pd = (printbuff*) data;
	FILE* fprint;
	fprint = fopen(term->cfg.fprintname, "wb");
	fwrite(prefix, prelen, 1, fprint);
	fflush(fprint);
	fwrite(pd->buf, pd->len, 1, fprint);
	fflush(fprint);
	fwrite(suffix, suflen, 1, fprint);
	fflush(fprint);
	fclose(fprint);
	sfree(pd->buf);
	sfree(pd);
	return 0;
}

//WINAPI 服务打印
static unsigned long printer_sprint_buff(void* data) {
	static char prefix[] = { 0x10, 0x42, 0x1B, 0x36, 0x1B, 0x28, 0x48, 0x00 };
	static int prelen = 8;
	static char suffix[] = { 0x0C };
	static int suflen = 1;
	printbuff* pd;
	pd = (printbuff*) data;
	if (prt_write(term->prtsClient, prefix, prelen) == 0) {
		prt_write(term->prtsClient, pd->buf, pd->len);
		prt_write(term->prtsClient, suffix, suflen);
	}

	sfree(pd->buf);
	sfree(pd);
	return 0;
}

typedef struct {
	int saved;
	bufchain* buf;
} printdata;

void printer_print_buff_init() {
	InitializeCriticalSection(&critsec);
	lpt = lptopen(0x378);
	if (lpt == (void*) 0) {
		MessageBox(NULL, ERROR_INITPRINT, TITLE_POINT, MB_ICONERROR | MB_OK);
	}
}

void printer_print_buff_end() {
	DeleteCriticalSection(&critsec);
	lptclose(lpt);
}

//WINAPI 直接打印
static unsigned long printer_dprint_data(void* data) {
	int len, size;
	void* pdata;
	printdata* pd;

	//EnterCriticalSection(&critsec);
	pd = (printdata*) data;
	if (lpt != (void*) 0) {
		while ((size = bufchain_size(pd->buf)) > pd->saved) {
			bufchain_prefix(pd->buf, &pdata, &len);
			if (len > size - pd->saved)
				len = size - pd->saved;
			lptwrite(lpt, pdata, len);
			bufchain_consume(pd->buf, len);
		}
		bufchain_clear(pd->buf);
		sfree(pd->buf);
		sfree(pd);
		//LeaveCriticalSection(&critsec);

		return 0;
	} else {
		MessageBox(NULL, ERROR_INITPRINT, TITLE_POINT, MB_ICONERROR | MB_OK);
		return -1;
	}
}

//WINAPI 文件打印
static unsigned long printer_fprint_data(void* data) {
	int len, size;
	void* pdata;
	printdata* pd;
	pd = (printdata*) data;
	FILE* fprint;
	fprint = fopen(term->cfg.fprintname, "wb");
	while ((size = bufchain_size(pd->buf)) > pd->saved) {
		bufchain_prefix(pd->buf, &pdata, &len);
		if (len > size - pd->saved)
			len = size - pd->saved;
		fwrite(pdata, len, 1, fprint);
		fflush(fprint);
		bufchain_consume(pd->buf, len);
	}
	fclose(fprint);
	bufchain_clear(pd->buf);
	sfree(pd->buf);
	sfree(pd);

	return 0;
}

//WINAPI 服务打印
static unsigned long printer_sprint_data(void* data) {
	int len, size;
	void* pdata;
	printdata* pd;
	pd = (printdata*) data;
	while ((size = bufchain_size(pd->buf)) > pd->saved) {
		bufchain_prefix(pd->buf, &pdata, &len);
		if (len > size - pd->saved)
			len = size - pd->saved;
		prt_write(term->prtsClient, pdata, len);
		bufchain_consume(pd->buf, len);
	}
	bufchain_clear(pd->buf);
	sfree(pd->buf);
	sfree(pd);

	return 0;
}

void printer_print_data_start(bufchain* buf, int saved) {
	printdata* pd;
	pd = smalloc(sizeof(printdata));
	pd->buf = buf;
	pd->saved = saved;
	if ((term->cfg.printmode == 2) && (term->cfg.allowrepeat == 0)) {
		printer_dprint_data(pd);
	} else if ((term->cfg.printmode == 2) && (term->cfg.allowrepeat == 1)) {
		printer_sprint_data(pd);
	} else if (term->cfg.printmode == 3) {
		printer_fprint_data(pd);
	}
	//CreateThread(NULL, 0, printer_print_data, pd, 0, 0);
}

void printer_print_buff_start(void* data, int len) {
	printbuff* pd = smalloc(sizeof(printbuff));
	pd->buf = data;
	pd->len = len;
	if ((term->cfg.printmode == 2) && (term->cfg.allowrepeat == 0)) {
		printer_dprint_buff(pd);
	} else if ((term->cfg.printmode == 2) && (term->cfg.allowrepeat == 1)) {
		printer_sprint_buff(pd);
	} else if (term->cfg.printmode == 3) {
		printer_fprint_buff(pd);
	}
	//CreateThread(NULL, 0, printer_print_buff, pd, 0, 0);
}
