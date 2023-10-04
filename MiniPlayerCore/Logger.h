#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <list>

//��־����
enum LogLevel {
	//����
	LogLevelText,
	//��Ϣ
	LogLevelInfo,
	//����
	LogLevelWarn,
	//����
	LogLevelError,
	//����
	LogLevelDisabled,
};
//��־���λ��
enum LogOutPut {
	//�����ϵͳĬ�Ͽ���̨
	LogOutPutConsolne,
	//������ļ�
	LogOutPutFile,
	//������Զ���ص�
	LogOutPutCallback,
};

typedef void(*LogCallBack)(const char* str, LogLevel level, void* lparam);

struct LOG_SLA {
	std::string str;
	LogLevel level;
};

//��־��¼
class Logger {

public:
	Logger(const char* tag);
	~Logger();

	static void InitConst();
	static void DestroyConst();
	static Logger* GetStaticInstance();

	virtual void Log(const char* str, ...);
	virtual void LogWarn(const char* str, ...);
	virtual void LogError(const char* str, ...);
	virtual void LogInfo(const char* str, ...);

	virtual void Log2(const char* str, const char* file, int line, const char* functon, ...);
	virtual void LogWarn2(const char* str, const char* file, int line, const char* functon, ...);
	virtual void LogError2(const char* str, const char* file, int line, const char* functon, ...);
	virtual void LogInfo2(const char* str, const char* file, int line, const char* functon, ...);

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
	LogLevel level = LogLevelText;
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

//���ټ�¼��־

#undef LOG 
#undef LOGI 
#undef LOGW 
#undef LOGE
#undef LOGD

#define LOG Logger::GetStaticInstance()
#define LOGI(fmt) Logger::GetStaticInstance()->LogInfo(fmt)
#define LOGW(fmt) Logger::GetStaticInstance()->LogWarn(fmt)
#define LOGE(fmt) Logger::GetStaticInstance()->LogError(fmt)
#define LOGD(fmt) Logger::GetStaticInstance()->Log(fmt)
#define LOGIF(fmt, ...) Logger::GetStaticInstance()->LogInfo(fmt, __VA_ARGS__)
#define LOGWF(fmt, ...) Logger::GetStaticInstance()->LogWarn(fmt, __VA_ARGS__)
#define LOGEF(fmt, ...) Logger::GetStaticInstance()->LogError(fmt, __VA_ARGS__)
#define LOGDF(fmt, ...) Logger::GetStaticInstance()->Log(fmt, __VA_ARGS__)

#define LogError2(str, ...) LogError2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
#define LogWarn2(str, ...) LogWarn2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
#define LogInfo2(str, ...) LogInfo2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
#define Log2(str, ...) Log2(str, __FILE__, __LINE__, __FUNCTION__,__VA_ARGS__)
