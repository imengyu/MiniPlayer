#include "pch.h"
#include "StringHelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

std::string& StringHelper::FormatString(std::string& _str, const char* format, ...) {
	std::string tmp;

	va_list marker = nullptr;
	va_start(marker, format);

#ifdef _MSC_VER
	size_t num_of_chars = _vscprintf(format, marker);
#else
	size_t num_of_chars = vsprintf(nullptr, format, marker);
#endif
	if (num_of_chars > tmp.capacity()) {
		tmp.resize(num_of_chars + 1);
	}
#if defined(_MSC_VER) && _MSC_VER > 1600
	vsprintf_s((char*)tmp.data(), tmp.capacity(), format, marker);
#else
	vsprintf(nullptr, format, marker);
#endif

	va_end(marker);

	_str = tmp.c_str();
	return _str;
}
void StringHelper::FreeString(LPCWSTR string) {
	delete string;
}
void StringHelper::FreeString(LPCSTR string) {
	delete string;
}
std::wstring& StringHelper::FormatString(std::wstring& _str, const wchar_t* format, ...) {
	std::wstring tmp;
	va_list marker = NULL;
	va_start(marker, format);
#ifdef _MSC_VER
	size_t num_of_chars = _vscwprintf(format, marker);
#else
	size_t num_of_chars = vswprintf(nullptr, 0, format, marker);
#endif
	if (num_of_chars > tmp.capacity()) {
		tmp.resize(num_of_chars + 1);
	}
#if defined(_MSC_VER) && _MSC_VER > 1600
	vswprintf_s((wchar_t*)tmp.data(), tmp.capacity(), format, marker);
#else
	vswprintf((wchar_t*)tmp.data(), tmp.capacity(), format, marker);
#endif

	va_end(marker);
	_str = tmp.c_str();
	return _str;
}
std::wstring StringHelper::FormatString(const wchar_t* format, va_list marker)
{
	std::wstring tmp;
#ifdef _MSC_VER
	size_t num_of_chars = _vscwprintf(format, marker);
#else
	size_t num_of_chars = vswprintf(nullptr, 0, format, marker);
#endif
	if (tmp.capacity() < num_of_chars + 1) {
		tmp.resize(num_of_chars + 1);
	}
#if defined(_MSC_VER) && _MSC_VER > 1600
	vswprintf_s((wchar_t*)tmp.data(), tmp.capacity(), format, marker);
#else
	vswprintf((wchar_t*)tmp.data(), tmp.capacity(), format, marker);
#endif
	return tmp;
}
std::string StringHelper::FormatString(const char* format, va_list marker)
{
	std::string tmp;
#ifdef _MSC_VER
	size_t num_of_chars = _vscprintf(format, marker);
#else
	size_t num_of_chars = vsprintf(nullptr, format, marker);
#endif
	if (tmp.capacity() < num_of_chars + 1) {
		tmp.resize(num_of_chars + 1);
	}

#if defined(_MSC_VER) && _MSC_VER > 1600
	vsprintf_s((char*)tmp.data(), tmp.capacity(), format, marker);
#else
	vsprintf(nullptr, format, marker);
#endif
	return tmp;
}
std::wstring StringHelper::FormatString(const wchar_t* format, ...)
{
	std::wstring tmp;
	va_list marker = NULL;
	va_start(marker, format);
#ifdef _MSC_VER
	size_t num_of_chars = _vscwprintf(format, marker);
#else
	size_t num_of_chars = vswprintf(nullptr, 0, format, marker);
#endif

	if (tmp.capacity() < num_of_chars + 1) {
		tmp.resize(num_of_chars + 1);
	}
#if defined(_MSC_VER) && _MSC_VER > 1600
	vswprintf_s((wchar_t*)tmp.data(), tmp.capacity(), format, marker);
#else
	vswprintf((wchar_t*)tmp.data(), tmp.capacity(), format, marker);
#endif
	va_end(marker);
	return tmp;
}
std::string StringHelper::FormatString(const char* format, ...)
{
	std::string tmp;

	va_list marker = NULL;
	va_start(marker, format);

#ifdef _MSC_VER
	size_t num_of_chars = _vscprintf(format, marker);
#else
	size_t num_of_chars = vsprintf(nullptr, format, marker);
#endif
	if (tmp.capacity() < num_of_chars + 1) {
		tmp.resize(num_of_chars + 1);
	}

#if defined(_MSC_VER) && _MSC_VER > 1600
	vsprintf_s((char*)tmp.data(), tmp.capacity(), format, marker);
#else
	vsprintf(nullptr, format, marker);
#endif

	va_end(marker);

	std::string _str = tmp.c_str();
	return _str;
}

void StringHelper::ReplaceStringChar(std::wstring& _str, wchar_t find, wchar_t replace) {
	for (size_t i = 0; i < _str.size(); i++)
	{
		if (_str[i] == find)
			_str[i] = replace;
	}
}
void StringHelper::ReplaceStringChar(std::string& _str, char find, char replace) {
	for (size_t i = 0; i < _str.size(); i++)
	{
		if (_str[i] == find)
			_str[i] = replace;
	}
}

std::wstring StringHelper::ReplaceString(std::wstring& _str, const wchar_t* find, const wchar_t* replace)
{
	std::wstring _find(find);
	std::wstring _replace(replace);
	return ReplaceString(_str, _find, _replace);
}

std::wstring StringHelper::ReplaceString(std::wstring& pszSrc, std::wstring& pszOld, std::wstring& pszNew)
{
	std::wstring strContent, strTemp;
	strContent.assign(pszSrc);
	std::wstring::size_type nPos = 0;
	while (true)
	{
		nPos = strContent.find(pszOld, nPos);
		strTemp = strContent.substr(nPos + pszOld.size(), strContent.length());
		if (nPos == std::wstring::npos)
		{
			break;
		}
		strContent.replace(nPos, strContent.length(), pszNew);
		strContent.append(strTemp);
		nPos += pszNew.size() - pszOld.size() + 1; //·ÀÖ¹ÖØ¸´Ìæ»» ±ÜÃâËÀÑ­»·
	}
	return strContent;
}
std::string StringHelper::ReplaceString(std::string& pszSrc, std::string& pszOld, std::string& pszNew)
{
	std::string strContent, strTemp;
	strContent.assign(pszSrc);
	std::string::size_type nPos = 0;
	while (true)
	{
		nPos = strContent.find(pszOld, nPos);
		strTemp = strContent.substr(nPos + pszOld.size(), strContent.length());
		if (nPos == std::string::npos)
		{
			break;
		}
		strContent.replace(nPos, strContent.length(), pszNew);
		strContent.append(strTemp);
		nPos += pszNew.size() - pszOld.size() + 1; //·ÀÖ¹ÖØ¸´Ìæ»» ±ÜÃâËÀÑ­»·
	}
	return strContent;
}

int StringHelper::StringToInt(std::string& _str) {
	return StringToInt(_str.c_str());
}
bool StringHelper::StringToBool(std::string& _str) {
	return StringToBool(_str.c_str());
}
int StringHelper::StringToInt(const char* _str) {
	return atoi(_str);
}
bool StringHelper::StringToBool(const char* _str) {
	if (strncmp(_str, "true", 4) == 0)
		return true;
	if (strncmp(_str, "TRUE", 4) == 0)
		return true;
	return atoi(_str) != 0;
}


char* StringHelper::AllocString(std::string& str)
{
	char* s = new char[str.size() + 1];
	strcpy_s(s, str.size() + 1, str.data());
	return s;
}
wchar_t* StringHelper::AllocString(std::wstring& str)
{
	wchar_t* s = new wchar_t[str.size() + 1];
	wcscpy_s(s, str.size() + 1, str.data());
	return s;
}
wchar_t* StringHelper::AllocString(std::wstring* str)
{
	wchar_t* s = new wchar_t[str->size() + 1];
	wcscpy_s(s, str->size() + 1, str->data());
	return s;
}

std::string StringHelper::UnicodeToAnsi(std::wstring szStr)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, szStr.c_str(), -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
		return NULL;
	std::string pResult;
	pResult.resize(nLen);
	WideCharToMultiByte(CP_ACP, 0, szStr.c_str(), -1, (LPSTR)pResult.data(), nLen, NULL, NULL);
	return pResult;
}
std::string StringHelper::UnicodeToUtf8(std::wstring unicode)
{
	int len;
	len = WideCharToMultiByte(CP_UTF8, 0, unicode.c_str(), -1, NULL, 0, NULL, NULL);
	std::string  szUtf8;
	szUtf8.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, unicode.c_str(), -1, (LPSTR)szUtf8.data(), len, NULL, NULL);
	return szUtf8;
}
std::wstring StringHelper::AnsiToUnicode(std::string szStr)
{
	int nLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szStr.c_str(), -1, NULL, 0);
	if (nLen == 0)
		return NULL;
	std::wstring  pResult;
	pResult.resize(nLen + 1);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szStr.c_str(), -1, (LPWSTR)pResult.data(), nLen);
	return pResult;
}
std::wstring StringHelper::Utf8ToUnicode(std::string szU8)
{
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8.c_str(), szU8.size(), NULL, 0);
	std::wstring wszString;
	wszString.resize(wcsLen + 1);
	::MultiByteToWideChar(CP_UTF8, NULL, szU8.c_str(), szU8.size(), (LPWSTR)wszString.data(), wcsLen);
	return wszString;
}

StringBuffer::StringBuffer(int size) {
	buffer = new char[size]; 
	bufferSize = size;
	memset(buffer, 0, size);
}
StringBuffer::~StringBuffer() {
	if (buffer) {
		delete buffer;
		buffer = nullptr;
	}
	bufferSize = 0;
	bufferCurrPos = 0;
}
void StringBuffer::PushChars(const char* chars, int align, int size) {
	int len = size > 0 ? size : strlen(chars);
	if (len <= 0)
		return;

	//Resize and copy
	if (bufferCurrPos + len >= bufferSize) {
		int oldBufferSize = bufferSize;
		char* oldBuffer = buffer;
		bufferSize = min(bufferSize + 32768, bufferSize * 2);
		buffer = new char[bufferSize];
		memset(buffer, 0, bufferSize);
		memcpy_s(buffer, bufferSize, oldBuffer, oldBufferSize);
		delete oldBuffer;
	}

	//Align
	if (align > 0 && bufferCurrPos % align > 0)
		bufferCurrPos += (align - bufferCurrPos % align);

	memcpy_s(buffer + bufferCurrPos, bufferSize, chars, len);
	bufferCurrPos += len;
}
int StringBuffer::GetBufferSize() {
	return bufferSize;
}
char* StringBuffer::GetBuffer() {
	return buffer;
}


