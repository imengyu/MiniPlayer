#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>
#ifndef MINI_PLAYER_EXPORT
#include "ExportDefine.h"
#endif

//日志级别
enum LogLevel {
	//文字
	LogLevelText,
	//信息
	LogLevelInfo,
	//警告
	LogLevelWarn,
	//错误
	LogLevelError,
	//禁用
	LogLevelDisabled,
};
//日志输出位置
enum LogOutPut {
	//输出到系统默认控制台
	LogOutPutConsolne,
	//输出到文件
	LogOutPutFile,
	//输出到自定义回调
	LogOutPutCallback,
};

typedef void(*LogCallBack)(const char* str, LogLevel level, void* lparam);

//日志记录
class MINI_PLAYER_EXPORT Logger {

public:
	static void InitConst();
	static void DestroyConst();
	static Logger* GetStaticInstance();

	virtual void Log(const char* str, ...) {}
	virtual void LogWarn(const char* str, ...) {}
	virtual void LogError(const char* str, ...) {}
	virtual void LogInfo(const char* str, ...) {}

	virtual void Log2(const char* str, const char* file, int line, const char* functon, ...) {}
	virtual void LogWarn2(const char* str, const char* file, int line, const char* functon, ...) {}
	virtual void LogError2(const char* str, const char* file, int line, const char* functon, ...) {}
	virtual void LogInfo2(const char* str, const char* file, int line, const char* functon, ...) {}

	virtual LogLevel GetLogLevel() { return LogLevel::LogLevelDisabled; }
	virtual void SetLogLevel(LogLevel logLevel) {}
	virtual void SetLogOutPut(LogOutPut output) {}
	virtual void SetLogOutPutCallback(LogCallBack callback, void* lparam) {}
	virtual void SetLogOutPutFile(const char* filePath) {}

	virtual void ResentNotCaputureLog() {}
	virtual void InitLogConsoleStdHandle() {}
};


#ifdef MINI_PLAYER_LIB

struct LOG_SLA {
	std::string str;
	LogLevel level;
};

//日志记录
class LoggerImpl : public Logger {

public:
	LoggerImpl(const char* tag);
	~LoggerImpl();

	void Log(const char* str, ...);
	void LogWarn(const char* str, ...);
	void LogError(const char* str, ...);
	void LogInfo(const char* str, ...);

	void Log2(const char* str, const char* file, int line, const char* functon, ...);
	void LogWarn2(const char* str, const char* file, int line, const char* functon, ...);
	void LogError2(const char* str, const char* file, int line, const char* functon, ...);
	void LogInfo2(const char* str, const char* file, int line, const char* functon, ...);

	LogLevel GetLogLevel();
	void SetLogLevel(LogLevel logLevel);
	void SetLogOutPut(LogOutPut output);
	void SetLogOutPutCallback(LogCallBack callback, void* lparam);
	void SetLogOutPutFile(const char* filePath);

	void ResentNotCaputureLog();
	void InitLogConsoleStdHandle();

private:

	std::list<LOG_SLA> logPendingBuffer;
	std::string logFilePath;
	std::string logTag;
	FILE* logFile = nullptr;
#if _DEBUG
	LogLevel level = LogLevelText;
#else
	LogLevel level = LogLevelWarn;
#endif
	LogOutPut outPut = LogOutPutConsolne;
	LogCallBack callBack = nullptr;
	void* callBackData;
	void* hOutput = NULL;

	void LogOutputToStdHandle(LogLevel logLevel, const char* str, size_t len);
	void WritePendingLog(const char* str, LogLevel logLevel);

	void LogInternalWithCodeAndLine(LogLevel logLevel, const char* str, const char* file, int line, const char* functon, va_list arg);
	void LogInternal(LogLevel logLevel, const char* str, va_list arg);
	void LogOutput(LogLevel logLevel, const char* str, const char* srcStr, size_t len);
	void CloseLogFile();
};

#endif

//快速记录日志

#undef LOG 
#undef LOGI 
#undef LOGW 
#undef LOGE
#undef LOGD

#define LOG Logger::GetStaticInstance()

#if _DEBUG
#define LOGI(fmt) Logger::GetStaticInstance()->LogInfo(fmt)
#define LOGD(fmt) Logger::GetStaticInstance()->Log(fmt)
#define LOGIF(fmt, ...) Logger::GetStaticInstance()->LogInfo(fmt, __VA_ARGS__)
#define LOGDF(fmt, ...) Logger::GetStaticInstance()->Log(fmt, __VA_ARGS__)
#define Log2(str, ...) Log2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
#else
#define LOGI(fmt)
#define LOGD(fmt)
#define LOGIF(fmt, ...)
#define LOGDF(fmt, ...)
#define Log2(str, ...)
#endif

#define LOGW(fmt) Logger::GetStaticInstance()->LogWarn(fmt)
#define LOGE(fmt) Logger::GetStaticInstance()->LogError(fmt)
#define LOGWF(fmt, ...) Logger::GetStaticInstance()->LogWarn(fmt, __VA_ARGS__)
#define LOGEF(fmt, ...) Logger::GetStaticInstance()->LogError(fmt, __VA_ARGS__)

#define LogError2(str, ...) LogError2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
#define LogWarn2(str, ...) LogWarn2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
#define LogInfo2(str, ...) LogInfo2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
