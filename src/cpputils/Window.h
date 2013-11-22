/*
 * Window.h
 *
 *  Created on: 2012-7-16
 *      Author: Administrator
 */

#ifndef WINDOW_H_
#define WINDOW_H_
#include <windows.h>
#include "Runnable.h"
#include "ThreadCreator.h"
#include <string>
using namespace std;

class Window {
private:
	string title;
	HWND hWnd;
	bool topMost;
	static WNDCLASS wndclass;

	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam,
			LPARAM lParam);
protected:

	virtual void OnPaint(HDC hDC);
public:
	Window(string title);
	virtual ~Window();
	void SetTopMost(bool topMost);
	void Create();
	void Show();
	void Close();
	void Refresh();

	void MoveWindow();

	void SetVisible(bool visiable);
	void Process();
};

#endif /* WINDOW_H_ */
