#include "stdafx.h"
#include "GUIControl.h"


GUIControl::GUIControl(const HWND hWnd) : hWnd_(hWnd)
{
	GetClientRect(hWnd_, &r_);
}

GUIControl::GUIControl(const HWND hWnd, const DWORD w, const DWORD h) : hWnd_(hWnd)
{
	GetClientRect(hWnd_, &r_);
	if (w)
		r_.right = r_.left + w;
	if (h)
		r_.bottom = r_.top + h;
}


GUIControl::~GUIControl(void)
{
}
