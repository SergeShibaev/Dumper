#pragma once
#include "stdafx.h"

class StatusBar
{
	HWND hParent_;
	HWND hStatus_;
	std::vector<int> sections_;
public:
	StatusBar() {}
	StatusBar(HWND hParent, int sections[], int amount);
	~StatusBar(void);
	void Resize(void);
};

