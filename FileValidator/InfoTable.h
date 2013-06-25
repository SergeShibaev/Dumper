#pragma once

class InfoTable
{
	HWND parent_;
	HWND dialog_;
	HWND hTable_;
	DWORD resourceId_;
	
public:
	InfoTable() { }
	InfoTable(HWND mainWnd, HWND dlgWnd, DWORD resId) : dialog_(dlgWnd), parent_(mainWnd), resourceId_(resId)
	{
		hTable_ = GetDlgItem(dialog_, resId);
		DeleteAllItems();
		ShowWindow(dialog_, SW_NORMAL);
		ShowWindow(hTable_, SW_NORMAL);
	}
	HWND GetHandle() { return hTable_; }
	void AddColumn(UINT mask, int fmt, int cx, LPTSTR pszText, int iSubitem);
	void InsertItem(size_t item, std::wstring pszText);
	void InsertSubitem(size_t item, DWORD subItem, std::wstring pszText);
	void AppendItem(std::wstring pszText);
	void AppendItem(std::wstring pszText, DWORD value);
	void AppendItem(std::wstring itemText, std::wstring subitemText);
	void Resize();
	DWORD GetItemCount() { return ListView_GetItemCount(hTable_); }
	void DeleteAllItems()
	{
		ListView_DeleteAllItems(hTable_);
	}
};
