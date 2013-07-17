#pragma once
#include <fstream>
#include "Dumper.h"

class Logger
{
	std::wstring logFileName_;
	std::wofstream logFile_;
	std::vector<std::wstring> log_;

	void InitLogFile();

public:
	Logger() {}
	Logger(std::wstring& logFileName) : logFileName_(logFileName) 
	{
		InitLogFile();
	}
	~Logger(void)
	{
		if (logFile_.is_open())
			logFile_.close();
	}

	void SetLogFile(const std::wstring& logFileName);
	void AddLog(const std::wstring& message);
	void AddLog(const std::wstring& message, const std::wstring& param);
	void Save();
};

