// FileValidator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "FakeDW.h"
#include "InfoTable.h"
#include "GUIControl.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND wndMain;
InfoTable table, dirs, sections, libs, importTable;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	MainWndProc(HWND, UINT, WPARAM, LPARAM);
void ResizeAllItems(HWND, HWND);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FILEVALIDATOR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FILEVALIDATOR));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FILEVALIDATOR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_FILEVALIDATOR);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 1100, 800, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   wndMain = CreateDialog(NULL, MAKEINTRESOURCE(IDD_MAINDIALOG), hWnd, MainWndProc);
   
   DWORD stText = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
   
   table = InfoTable(hWnd, wndMain, IDC_TABLE);
   table.AddColumn(stText, LVCFMT_LEFT, 200, L"Item", 0);
   table.AddColumn(stText, LVCFMT_LEFT, 300, L"Value", 1);
   
   dirs = InfoTable(hWnd, wndMain, IDC_DIRS);
   dirs.AddColumn(stText, LVCFMT_LEFT, 20, L"N", 0);
   dirs.AddColumn(stText, LVCFMT_LEFT, 250, L"Директория", 1);
   dirs.AddColumn(stText, LVCFMT_RIGHT, 85, L"Адрес", 2);
   dirs.AddColumn(stText, LVCFMT_RIGHT, 60, L"Размер", 3);
   dirs.AddColumn(stText, LVCFMT_LEFT, 80, L"Секция", 4);   

   sections = InfoTable(hWnd, wndMain, IDC_SECTIONS);
   sections.AddColumn(stText, LVCFMT_LEFT, 25, L"ID", 0);
   sections.AddColumn(stText, LVCFMT_LEFT, 50, L"Секция", 1);
   sections.AddColumn(stText, LVCFMT_RIGHT, 85, L"VirtualAddress", 2);
   sections.AddColumn(stText, LVCFMT_RIGHT, 80, L"VirtualSize", 3);   
   sections.AddColumn(stText, LVCFMT_RIGHT, 105, L"PointerToRawData", 4);
   
   libs = InfoTable(hWnd, wndMain, IDC_LIBRARIES);
   libs.AddColumn(stText, LVCFMT_LEFT, 500, L"Библиотека", 0);

   importTable = InfoTable(hWnd, wndMain, IDC_SECTIONLIST);
   importTable.AddColumn(stText, LVCFMT_LEFT, 100, L"Библиотека", 0);
   importTable.AddColumn(stText, LVCFMT_LEFT, 200, L"Функция", 1);
   importTable.AddColumn(stText, LVCFMT_LEFT, 75, L"Ordinal", 2);
   importTable.AddColumn(stText, LVCFMT_LEFT, 75, L"Hint", 3);
   importTable.AddColumn(stText, LVCFMT_LEFT, 50, L"Проверка", 4);

   ResizeAllItems(hWnd, wndMain);
   ShowWindow(wndMain, SW_MAXIMIZE);
   return TRUE;
}

BOOL GetOpenFileName(HWND hWnd, std::wstring& fileName)
{
	OPENFILENAME ofn;	
	wchar_t FileName[MAX_PATH] = { 0 };

	ZeroMemory(&FileName, sizeof(FileName));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = TEXT("Executable files (*.exe)\0*.exe\0Libraries (*.dll)\0*.dll\0");
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = TEXT("Выбор файла");
	ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_FORCESHOWHIDDEN;

	GetOpenFileName(&ofn);

	if (ofn.lpstrFile[0] != '\0')
	{
		fileName = ofn.lpstrFile;
		return TRUE;
	}
	else
		return FALSE;
}

void Dump(std::wstring fileName)
{
	WCHAR curDir[MAX_PATH];
	Dumper::SplitPath((LPWSTR)fileName.c_str(), curDir, NULL);
	SetCurrentDirectory(curDir);
	
	Dumper dumper(fileName, TRUE, TRUE);
	dumper.ShowHeader(table);
	dumper.ShowSections(sections);	
	dumper.ShowDataDirs(dirs);	
	dumper.ShowImportTable(importTable);
	dumper.CheckDependencies();
	dumper.ShowLibraries(libs);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	std::wstring fileName;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_FILE_DUMP:
			if (GetOpenFileName(hWnd, fileName))
				Dump(fileName);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_SIZE:
		ResizeAllItems(hWnd, wndMain);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK MainWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		break;
	}
	return (INT_PTR)FALSE;
}

void ResizeAllItems(HWND hWnd, HWND mainWnd)
{
	if (!hWnd || !mainWnd)
		return;
	
	RECT r;
	GetClientRect(hWnd, &r);
	
	GUIControl main(mainWnd, r.right, r.bottom);
	main.Move(0, 0);
	
	// left pane
	DWORD lcHeight = main.height();
	DWORD lcWidth = main.width()/2;
	GUIControl stHeader(GetDlgItem(mainWnd, IDC_STATICHEADER), lcWidth, 15);
	stHeader.Move(0, 0);
	lcHeight -= stHeader.height();
	
	GUIControl tblDirs(GetDlgItem(mainWnd, IDC_DIRS), lcWidth, 150);
	tblDirs.Move(0, main.height()-tblDirs.height());
	lcHeight -= tblDirs.height();

	GUIControl stDirs(GetDlgItem(mainWnd, IDC_STATICDIRS), lcWidth, 15);	
	stDirs.Move(0, tblDirs.GetTop()-stDirs.height());
	lcHeight -= stDirs.height();

	GUIControl tblSections(GetDlgItem(mainWnd, IDC_SECTIONS), lcWidth, 150);
	tblSections.Move(0, stDirs.GetTop()-tblSections.height());
	lcHeight -= tblSections.height();
	
	GUIControl stSections(GetDlgItem(mainWnd, IDC_STATICSECTIONS), lcWidth, 15);
	stSections.Move(0, tblSections.GetTop()-stSections.height());
	lcHeight -= stSections.height();

	GUIControl tblHeader(GetDlgItem(mainWnd, IDC_TABLE), main.width()/2, lcHeight);
	tblHeader.left(0)->Move(0, stHeader.GetBottom());

	// right pane
	DWORD rcHeight = main.height();
	DWORD rcLeft = main.width()/2;
	DWORD rcWidth = main.width()/2;
	
	GUIControl stLibs(GetDlgItem(mainWnd, IDC_STATICLIBRARIES), rcWidth, 15);
	stLibs.Move(rcLeft, 0);
	rcHeight -= stLibs.height();

	GUIControl tblLibs(GetDlgItem(mainWnd, IDC_LIBRARIES), rcWidth, 200);
	tblLibs.Move(rcLeft, stLibs.GetBottom());
	rcHeight -= tblLibs.height();

	GUIControl stDepends(GetDlgItem(mainWnd, IDC_STATICDEPENDS), rcWidth, 15);
	stDepends.Move(rcLeft, tblLibs.GetBottom());
	rcHeight -= stDepends.height();

	GUIControl tblImport(GetDlgItem(mainWnd, IDC_IMPORT), rcWidth, rcHeight);
	tblImport.Move(rcLeft, stDepends.GetBottom());
}