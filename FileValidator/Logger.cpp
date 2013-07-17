#include "stdafx.h"
#include "Logger.h"

void Logger::InitLogFile()
{
	WCHAR logPath[MAX_PATH] = { 0 }, fileName[MAX_PATH], filePath[MAX_PATH];

	GetModuleFileName(NULL, logPath, MAX_PATH);
	Dumper::SplitPath(logPath, filePath, fileName);
	logFileName_ = std::wstring(filePath) + L"\\" + logFileName_;

	logFile_.open(logFileName_.c_str(), std::ios_base::trunc | std::ios_base::out);
	logFile_.imbue(std::locale("Russian"));
}

void Logger::SetLogFile(const std::wstring& logFileName)
{
	logFileName_ = logFileName;
	InitLogFile();
}

void Logger::AddLog(const std::wstring& message)
{
	/*if (logFile_.is_open())
		logFile_ << message.c_str() << L"\n";*/
	log_.push_back(message);
}

void Logger::AddLog(const std::wstring& message, const std::wstring& param)
{

}

void Logger::Save()
{
	if (logFile_.is_open())
	{
		for (size_t i = 0; i < log_.size(); ++i)
			logFile_ << log_[i].c_str() << L"\n";
	}
}