/*
 * printapi.cpp
 *
 *  Created on: 2013-9-11
 *      Author: zhangbo
 */

#include "printapi.h"

#include "putty.h"
#include "util/windows/print.h"
#include "util/windows/winlock.h"
#include <stdlib.h>
#include "util/log.h"

typedef struct printer {
	char type; //0 直接打印 1 驱动打印 2 共享打印 3 文件打印
	char wholedoc; //4i5i是否规整，如果规整，一个5i4i代表一个文档，否则，整个应用只打开一个文档。
	int buflen; //缓存大小
	char buffer[32 * 1024 * 1024]; //缓存
	void* handle;
	bufchain chain;
	void* lock;
	char name[1024];
	BOOL running;
} PRINTER, *PPRINTER;

static unsigned long WINAPI print_thread(void* printer) {
	PPRINTER handle = (PPRINTER) printer;
	void* buf;
	int len;
	handle->running = TRUE;

	log_write("PRT_THRD", "STAR", 4);
	while (handle->running) {
		lock(handle->lock);
		len = 0;
		if (bufchain_size(&handle->chain) > 0) {
			bufchain_prefix(&handle->chain, &buf, &len);
		}
		unlock(handle->lock);
		if (len > 0) {
			log_write(PRT_ENDS, buf, len);
			lptwrite(handle->handle, buf, len);
			lock(handle->lock);
			bufchain_consume(&handle->chain, len);
			unlock(handle->lock);
		} else {
			Sleep(10);
		}
	}
	log_write("PRT_THRD", "STOP", 4);
	return 0;
}

void* print_new(char *name, char type, char whole) {
	PPRINTER handle = (PPRINTER) malloc(sizeof(PRINTER));
	memset(handle, 0, sizeof(PRINTER));
	handle->type = type; //记录打印类型
	handle->wholedoc = whole; //农信5i-4i并不是规范。

	switch (handle->type) {
	case PRINT_LPT: //直接打印
		handle->handle = lptopen(0x378);
		handle->buflen = 0;
		handle->lock = create_lock();
		CreateThread(NULL, 0, print_thread, handle, 0, 0);
		break;
	case PRINT_DRV: //驱动打印
		if (handle->wholedoc) {
			handle->handle = (void*) printer_start_job(name);
		} else {
			strcpy(handle->name, name);
		}
		break;
	case PRINT_SHR: //共享打印
		break;
	case PRINT_COM: //串口打印
		break;
	case PRINT_FIL: //文件打印
		if (handle->wholedoc) {
			handle->handle = fopen(name, "wb");
		} else {
			strcpy(handle->name, name);
		}
		break;
	}
	return handle;
}

void print_del(void* printer) {
	PPRINTER handle = (PPRINTER) printer;
	switch (handle->type) {
	case PRINT_LPT: //直接打印
		if (handle->handle != NULL) {
			lptclose(handle->handle);
			handle->handle = NULL;
			handle->buflen = 0;
		}
		handle->running = FALSE;
		release_lock(handle->lock);
		break;
	case PRINT_DRV: //驱动打印
		if (handle->handle != NULL) {
			printer_finish_job(handle->handle);
			handle->handle = NULL;
		}
		break;
	case PRINT_SHR: //共享打印
		break;
	case PRINT_COM: //串口打印
		break;
	case PRINT_FIL: //文件打印
		if (handle->handle != NULL) {
			fclose(handle->handle);
			handle->handle = NULL;
		}
		break;
	}

	free(handle);
}

void print_open(void* printer) {
	PPRINTER handle = (PPRINTER) printer;
	switch (handle->type) {
	case PRINT_LPT: //直接打印
		handle->buflen = 0;
		break;
	case PRINT_DRV: //驱动打印
		if (!handle->wholedoc) {
			if (handle->handle != NULL) {
				printer_finish_job(handle->handle);
			}
			handle->handle = (void*) printer_start_job(handle->name);
		}
		break;
	case PRINT_SHR: //共享打印
		break;
	case PRINT_COM: //串口打印
		break;
	case PRINT_FIL: //文件打印
		if (handle->handle == NULL) {
			handle->handle = (void*) fopen(handle->name, "wb");
		}
		break;
	}
}

void print_data(void* printer, void* data, int len) {
	PPRINTER handle = (PPRINTER) printer;
	switch (handle->type) {
	case PRINT_LPT: //直接打印
		memcpy(handle->buffer + handle->buflen, data, len);
		handle->buflen += len;
		break;
	case PRINT_DRV: //驱动打印
		printer_job_data((printer_job *) (handle->handle), data, len);
		break;
	case PRINT_SHR: //共享打印
		break;
	case PRINT_COM: //串口打印
		break;
	case PRINT_FIL: //文件打印
		if (handle->handle) {
			fwrite(data, len, 1, (FILE*) handle->handle);
			fflush((FILE*) handle->handle);
		}
		break;
	}
}

void print_close(void* printer) {
	PPRINTER handle = (PPRINTER) printer;
	switch (handle->type) {
	case PRINT_LPT: //直接打印
		lock(handle->lock);
		bufchain_add(&handle->chain, handle->buffer, handle->buflen);
		unlock(handle->lock);
		handle->buflen = 0;
		break;
	case PRINT_DRV: //驱动打印
		if (!handle->wholedoc && handle->handle != NULL) {
			printer_finish_job((printer_job*) handle->handle);
			handle->handle = NULL;
		}
		break;
	case PRINT_SHR: //共享打印
		break;
	case PRINT_COM: //串口打印
		break;
	case PRINT_FIL: //文件打印
		fflush(handle->handle);
		if (!handle->wholedoc) {
			fclose(handle->handle);
			handle->handle = NULL;
		}
		break;
	}
}

void print_file(void* printer, char* file) {
	FILE *fp = fopen(file, "rb");

	char buf[1024];
	while (TRUE) {
		size_t s = fread(buf, 1, 1024, fp);
		if (s > 0) {
			print_data(printer, buf, s);
		} else {
			break;
		}
	}
	fclose(fp);
}
