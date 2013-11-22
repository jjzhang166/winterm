#ifndef WINIO_H
#define WINIO_H

#ifdef WINIO_DLL
#define WINIO_API _declspec(dllexport)
#else
#define WINIO_API
//_declspec(dllimport)
#endif

#include <windows.h>

#ifdef  __cplusplus
extern "C" {
#endif

WINIO_API _stdcall BOOL InitializeWinIo();
WINIO_API void _stdcall ShutdownWinIo();
WINIO_API PBYTE _stdcall MapPhysToLin(PBYTE pbPhysAddr, DWORD dwPhysSize,
		HANDLE *pPhysicalMemoryHandle);
WINIO_API BOOL _stdcall UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle,
		PBYTE pbLinAddr);
WINIO_API BOOL _stdcall GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal);
WINIO_API BOOL _stdcall SetPhysLong(PBYTE pbPhysAddr, DWORD dwPhysVal);
WINIO_API BOOL _stdcall GetPortVal(WORD wPortAddr, PDWORD pdwPortVal,
		BYTE bSize);
WINIO_API BOOL _stdcall SetPortVal(WORD wPortAddr, DWORD dwPortVal, BYTE bSize);
WINIO_API BOOL _stdcall InstallWinIoDriver(PSTR pszWinIoDriverPath,
		BOOL IsDemandLoaded);// = FALSE
WINIO_API BOOL _stdcall RemoveWinIoDriver();

#ifdef  __cplusplus
}
#endif

extern BOOL IsNT;
extern HANDLE hDriver;
extern BOOL IsWinIoInitialized;

_stdcall BOOL StartWinIoDriver();
_stdcall BOOL StopWinIoDriver();

#endif
