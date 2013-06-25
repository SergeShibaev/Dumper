#include "stdafx.h"

std::wstring Convert( const std::string& str )
{
	std::wstring result;
	result.resize(str.length());
	size_t convCharsCount = 0;
	mbstowcs_s(&convCharsCount, &result[0], result.length()+1, str.c_str(), str.length());

	return result;
}

std::string Convert( const std::wstring& str )
{
	std::string result;
	result.resize(str.length());
	size_t convCharsCount = 0;
	wcstombs_s(&convCharsCount, &result[0], result.length()+1, str.c_str(), str.length());

	return result;
}

std::wstring ValueAsStr(DWORD value)
{
	WCHAR str[10];
	StringCchPrintf(str, 10, L"%d", value);
	return str;
}

std::wstring ValueAsHex(DWORD value)
{
	WCHAR str[16];
	StringCchPrintf(str, 16, L"0x%X", value);
	return str;
}