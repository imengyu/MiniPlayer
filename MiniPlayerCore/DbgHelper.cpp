#include "pch.h"
#include "DbgHelper.h"
#include "StringHelper.h"
#include <stdarg.h>

void DbgPrintf(wchar_t const * const _Format, ...)
{
	va_list marker = NULL;
	va_start(marker, _Format);
	std::wstring w = StringHelper::FormatString(_Format, marker);
	OutputDebugString(w.c_str());
	va_end(marker);
}
