#include "stdafx.h"
#include "StatusBar.h"


StatusBar::StatusBar(HWND hParent, int *sections, int amount) : 
	hParent_(hParent)
{
	for (int i = 0; i < amount; ++i)
		sections_.push_back(sections[i]);
    hStatus_ = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, 
	   hParent, NULL, GetModuleHandle(NULL), NULL);   
    SendMessage(hStatus_, SB_SETPARTS, (WPARAM)amount, (LPARAM)sections);
    //SendMessage(hStatus_, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)L"Test");
}

StatusBar::~StatusBar(void)
{
}

void StatusBar::Resize()
{
	RECT r;
	GetClientRect(hParent_, &r);
}