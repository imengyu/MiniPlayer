#pragma once
#include "pch.h"
#include "ExportDefine.h"
#include <string>
#include <exception>

#define Assert(expression) DbgAssert((int)(expression), __FILE__, __LINE__, #expression)
#define Assert2 (expression, messasge) DbgAssert((int)(expression), __FILE__, __LINE__, messasge)

void MINI_PLAYER_EXPORT DbgAssert(int condition, const char* file, int line, const char* message);

void DbgPrintf(_In_z_ _Printf_format_string_ wchar_t const* const _Format, ...);