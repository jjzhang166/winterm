/*
 * Window.cpp
 *
 *  Created on: 2012-7-16
 *      Author: Administrator
 */

#include "Window.h"
#include "Util.h"
#include "WindowProcessor.h"
#include <stdio.h>

WNDCLASS Window::wndclass;

Window::Window(string title) {
	this->hWnd = NULL;
	this->title = title;
}

Window::~Window() {
}

struct PARAM {
	Window* pWindow;
	LPARAM lParam;
};

LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT message, WPARAM wParam,
		LPARAM lParam) {

	switch (message) {
	case WM_CREATE: {
		return 0;
	}
	case WM_PAINT: {
		if (lParam != 0) {
			PARAM * pParam = (PARAM*) lParam;
			lParam = pParam->lParam;
			Window* pWindow = pParam->pWindow;
			delete pParam;
			HDC hDC;
			PAINTSTRUCT ps;
			hDC = BeginPaint(hwnd, &ps);
			pWindow->OnPaint(hDC);
			EndPaint(hwnd, &ps);
		}
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

void Window::Create() {
	WindowProcessor* processor = new WindowProcessor();
	processor->SetWindow(this);
	ThreadCreator::StartThread(processor);
}

void Window::Show() {
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void Window::Process() {
	static TCHAR szAppName[] = TEXT("HelloWin");

	if (wndclass.lpfnWndProc != WndProc) {
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = WndProc;
		wndclass.cbClsExtra = 0;
		wndclass.cbWndExtra = 0;
		wndclass.hInstance = NULL;
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName = NULL;
		wndclass.lpszClassName = szAppName;
		wndclass.cbClsExtra = 0;
		if (!RegisterClass(&wndclass)) {
			return;
		}
	}

	hWnd = CreateWindow(
			szAppName, // window class name
			this->title.c_str(), WS_POPUP, 0, 0,
			::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN),
			NULL,// parent window handle
			NULL,// window menu handle
			NULL,// program instance handle
			NULL);

	if (this->topMost) {
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);

		if (msg.message == WM_PAINT) {
			PARAM* pParam = new PARAM;
			pParam->pWindow = this;
			pParam->lParam = msg.lParam;
			msg.lParam = (LPARAM) pParam;
		}

		DispatchMessage(&msg);
	}
}

void Window::Close() {
	SendMessage(hWnd, WM_CLOSE, 0, 0);
	UnregisterClass(wndclass.lpszClassName, NULL);
	hWnd = NULL;
}

void Window::Refresh() {
	InvalidateRect(hWnd, NULL, true);
}

void Window::OnPaint(HDC hDC) {
}

void Window::SetTopMost(bool topMost) {
	this->topMost = topMost;
	if (hWnd != NULL) {
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}

void Window::MoveWindow() {
	ScrollWindow(hWnd, 0, -100, NULL, NULL);
	Sleep(1000);
	UpdateWindow(hWnd);
}

void Window::SetVisible(bool visiable) {
	if (visiable) {
		ShowWindow(hWnd, SW_SHOW);
	} else {
		ShowWindow(hWnd, SW_HIDE);
	}
	UpdateWindow(hWnd);
}
