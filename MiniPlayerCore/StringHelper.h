#pragma once
#ifndef MINI_PLAYER_EXPORT
#include "ExportDefine.h"
#endif
#include <stdio.h>  
#include <stdlib.h>  
#include <string>  
#include <vector>

class MINI_PLAYER_EXPORT StringHelper
{
public:
	static std::string& FormatString(std::string& _str, const char* format, ...);
	static std::wstring& FormatString(std::wstring& _str, const wchar_t* format, ...);
	static std::wstring FormatString(const wchar_t* format, ...);
	static std::wstring FormatString(const wchar_t* format, va_list marker);
	static std::string FormatString(const char* format, va_list marker);
	static std::string FormatString(const char* format, ...);

	static void ReplaceStringChar(std::wstring& _str, wchar_t find, wchar_t replace);
	static void ReplaceStringChar(std::string& _str, char find, char replace);
	static std::wstring ReplaceString(std::wstring& _str, const wchar_t* find, const wchar_t* replace);
	static std::wstring ReplaceString(std::wstring& _str, std::wstring& find, std::wstring& replace);
	static std::string ReplaceString(std::string& _str, std::string& find, std::string& replace);

	static char* AllocString(std::string& str);
	static wchar_t* AllocString(std::wstring& str);
	static wchar_t* AllocString(std::wstring* str);

	static int StringToInt(std::string& _str);
	static bool StringToBool(std::string& _str);
	static int StringToInt(const char* _str);
	static bool StringToBool(const char* _str);

	static bool EndWith(const wchar_t* string, const wchar_t* string2);
	static bool EndWith(const char* string, const char* string2);

	static void FreeString(const wchar_t* string);
	static void FreeString(const char* string);

	static std::string UnicodeToAnsi(std::wstring szStr);
	static std::string UnicodeToUtf8(std::wstring unicode);
	static std::wstring AnsiToUnicode(std::string szStr);
	static std::wstring Utf8ToUnicode(std::string szU8);
};

class MINI_PLAYER_EXPORT StringBuffer {
public:
	StringBuffer(int size);
	~StringBuffer();

	void PushChars(const char* chars, int align = 0, int size = 0);
	int GetBufferSize();
	char* GetBuffer();

private:
	int bufferSize = 0;
	int bufferCurrPos = 0;
	char* buffer = nullptr;
};







