#pragma once
class GUIControl
{
	RECT r_;
	HWND hWnd_;
	HWND hParent_;	
public:
	GUIControl(const HWND hWnd);
	GUIControl(const HWND hWnd, const DWORD w, const DWORD h);
	~GUIControl(void);
	DWORD width() { return r_.right - r_.left; }
	DWORD height() { return r_.bottom - r_.top; }

	GUIControl* const left(DWORD pos) { r_.left = pos; return this; }
	GUIControl* const top(DWORD pos) { r_.top = pos; return this; }
	GUIControl* const right(DWORD pos) { r_.right = pos; return this; }
	GUIControl* const bottom(DWORD pos) { r_.bottom = pos; return this; }
	GUIControl* const width(DWORD w) { r_.right = r_.left + w; return this; }
	GUIControl* const rwidth(DWORD w) { r_.left = r_.right - w; return this; }
	GUIControl* const height(const DWORD h) { r_.bottom = r_.top + h; return this; }

	DWORD GetTop() const { return r_.top; }
	DWORD GetBottom() const { return r_.bottom; }
	DWORD GetRight() const { return r_.right; }

	void Move() 
	{ 
		MoveWindow(hWnd_, r_.left, r_.top, this->width(), this->height(), TRUE); 
	}
	void Move(const DWORD left, const DWORD top) 
	{
		r_.left = left; r_.top = top; 
		r_.right += left; r_.bottom += top;
		Move();
	}
};

