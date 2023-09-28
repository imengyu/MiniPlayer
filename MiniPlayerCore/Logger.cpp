#include "pch.h"
#include "Logger.h"
#include "StringHelper.h"
#include <ctime>

using namespace std;

#undef LogError2
#undef LogWarn2
#undef LogInfo2
#undef Log2

Logger* globalStaticLogger = nullptr;

void Logger::InitConst() { globalStaticLogger = new Logger("App"); }
void Logger::DestroyConst() { delete globalStaticLogger; }

Logger* Logger::GetStaticInstance()
{
	return globalStaticLogger;
}

Logger::Logger(const char* tag)
{
	logTag = tag;
}
Logger::~Logger()
{
	CloseLogFile();
}

void Logger::Log(const char* str, ...)
{
	if (level <= LogLevelText) {
		va_list arg;
		va_start(arg, str);
		LogInternal(LogLevelText, str, arg);
		va_end(arg);
	}
}
void Logger::LogWarn(const char* str, ...)
{
	if (level <= LogLevelWarn) {
		va_list arg;
		va_start(arg, str);
		LogInternal(LogLevelWarn, str, arg);
		va_end(arg);
	}
}
void Logger::LogError(const char* str, ...)
{
	if (level <= LogLevelError) {
		va_list arg;
		va_start(arg, str);
		LogInternal(LogLevelError, str, arg);
		va_end(arg);
	}
}
void Logger::LogInfo(const char* str, ...)
{
	if (level <= LogLevelInfo) {
		va_list arg;
		va_start(arg, str);
		LogInternal(LogLevelInfo, str, arg);
		va_end(arg);
	}
}

void Logger::Log2(const char* str, const char* file, int line, const char* functon, ...)
{
	if (level <= LogLevelText) {
		va_list arg;
		va_start(arg, functon);
		LogInternalWithCodeAndLine(LogLevelText, str, file, line, functon, arg);
		va_end(arg);
	}
}
void Logger::LogWarn2(const char* str, const char* file, int line, const char* functon, ...)
{
	if (level <= LogLevelWarn) {
		va_list arg;
		va_start(arg, functon);
		LogInternalWithCodeAndLine(LogLevelWarn, str, file, line, functon, arg);
		va_end(arg);
	}
}
void Logger::LogError2(const char* str, const char* file, int line, const char* functon, ...)
{
	if (level <= LogLevelError) {
		va_list arg;
		va_start(arg, functon);
		LogInternalWithCodeAndLine(LogLevelError, str, file, line, functon, arg);
		va_end(arg);
	}
}
void Logger::LogInfo2(const char* str, const char* file, int line, const  char* functon, ...)
{
	if (level <= LogLevelInfo) {
		va_list arg;
		va_start(arg, functon);
		LogInternalWithCodeAndLine(LogLevelInfo, str, file, line, functon, arg);
		va_end(arg);
	}
}

void Logger::SetLogLevel(LogLevel logLevel)
{
	this->level = logLevel;
}
LogLevel Logger::GetLogLevel() {
	return this->level;
}
void Logger::SetLogOutPut(LogOutPut output)
{
	this->outPut = output;
}
void Logger::SetLogOutPutFile(const char* filePath)
{
	if (logFilePath != filePath)
	{
		CloseLogFile();
		logFilePath = filePath;
#if defined(_MSC_VER) && _MSC_VER > 1600
		fopen_s(&logFile, (char*)logFilePath.data(), "w");
#else
		logFile = _wfopen(logFilePath.data(), L"w");
#endif
	}
}
void Logger::SetLogOutPutCallback(LogCallBack callback, void* lparam)
{
	callBack = callback;
	callBackData = lparam;
}
void Logger::InitLogConsoleStdHandle() {
	hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
}
void Logger::LogOutputToStdHandle(LogLevel logLevel, const char* str, size_t len) {
	switch (logLevel)
	{
	case LogLevelInfo:
		SetConsoleTextAttribute(hOutput, FOREGROUND_INTENSITY | FOREGROUND_BLUE);
		break;
	case LogLevelWarn:
		SetConsoleTextAttribute(hOutput, FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN);
		break;
	case LogLevelError:
		SetConsoleTextAttribute(hOutput, FOREGROUND_INTENSITY | FOREGROUND_RED);
		break;
	case LogLevelText:
		SetConsoleTextAttribute(hOutput, FOREGROUND_INTENSITY | FOREGROUND_RED |
			FOREGROUND_GREEN |
			FOREGROUND_BLUE);
		break;
	}
	WriteConsoleA(hOutput, str, len, NULL, NULL);
}

void Logger::ResentNotCaputureLog()
{
	if (outPut == LogOutPutCallback && callBack) {
		std::list< LOG_SLA>::iterator i;
		for (i = logPendingBuffer.begin(); i != logPendingBuffer.end(); i++)
			callBack((*i).str.c_str(), (*i).level, callBackData);
		logPendingBuffer.clear();
	}
}
void Logger::WritePendingLog(const char* str, LogLevel logLevel)
{
	LOG_SLA sla = { std::string(str), logLevel };
	logPendingBuffer.push_back(sla);
}

void Logger::LogInternalWithCodeAndLine(LogLevel logLevel, const char* str, const char* file, int line, const char* functon, va_list arg)
{
	std::string format1 = StringHelper::FormatString("%s\n[In] %s (%d) : %hs", str, file, line, functon);
	LogInternal(logLevel, format1.c_str(), arg);
}
void Logger::LogInternal(LogLevel logLevel, const char* str, va_list arg)
{
	const char* levelStr;
	switch (logLevel)
	{
	case LogLevelInfo: levelStr = "I"; break;
	case LogLevelWarn: levelStr = "W"; break;
	case LogLevelError: levelStr = "E"; break;
	case LogLevelText: levelStr = "T"; break;
	default: levelStr = ""; break;
	}
	time_t time_log = time(NULL);
#if defined(_MSC_VER) && _MSC_VER > 1600
	struct tm tm_log;
	localtime_s(&tm_log, &time_log);
	std::string format1 = StringHelper::FormatString("[%02d:%02d:%02d] [%s] %s\n", tm_log.tm_hour, tm_log.tm_min, tm_log.tm_sec, levelStr, str);
#else
	tm* tm_log = localtime(&time_log);
	std::string format1 = StringHelper::FormatString("%s/%s:%02d:%02d:%02d %s\n", logTag.c_str(), levelStr, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec, str);
#endif
	std::string out = StringHelper::FormatString(format1.c_str(), arg);
	LogOutput(logLevel, out.c_str(), str, out.size());
}
void Logger::LogOutput(LogLevel logLevel, const char* str, const char* srcStr, size_t len)
{
#if _DEBUG
	OutputDebugStringA(str);
#else
	if (outPut == LogOutPutConsolne)
		OutputDebugStringA(str);
#endif
	if (outPut == LogOutPutFile && logFile)
		fprintf_s(logFile, "%s", str);
	else if (outPut == LogOutPutConsolne) {
		if (hOutput != NULL) LogOutputToStdHandle(logLevel, str, len);
		else printf_s("%s", str);
	}
	else if (outPut == LogOutPutCallback && callBack)
		callBack(str, logLevel, callBackData);
	else
		WritePendingLog(str, logLevel);
}
void Logger::CloseLogFile()
{
	if (logFile) {
		fclose(logFile);
		logFile = nullptr;
	}
}