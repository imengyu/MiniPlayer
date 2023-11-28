#include "pch.h"
#include "DbgHelper.h"
#include "StringHelper.h"
#include <stdarg.h>

void DbgAssert(int condition, const char* file, int line, const char* message)
{
	if (!condition) {
		auto finalMessage = StringHelper::FormatString("Assertion failed! %s at %s line %d", message, file, line);

#if _DEBUG
		OutputDebugStringA(finalMessage.c_str());
		DebugBreak();
#endif
		MessageBoxA(GetForegroundWindow(), finalMessage.c_str(), "Error", MB_ICONERROR);
		exit(-1);
	}
}

void DbgPrintf(wchar_t const * const _Format, ...)
{
	va_list marker = NULL;
	va_start(marker, _Format);
	std::wstring w = StringHelper::FormatString(_Format, marker);
	OutputDebugString(w.c_str());
	va_end(marker);
}


