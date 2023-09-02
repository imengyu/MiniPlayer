#include "pch.h"
#include "CSoundDecoder.h"
#include "StringHelper.h"

CSoundDecoder::CSoundDecoder()
{
}

void CSoundDecoder::SetLastError(int code, const wchar_t* errmsg)
{
	lastErrorMessage = errmsg ? errmsg : L"";
	lastErrorCode = code;
}
void CSoundDecoder::SetLastError(int code, const char* errmsg)
{
	lastErrorMessage = errmsg ? StringHelper::AnsiToUnicode(errmsg) : L"";
	lastErrorCode = code;
}
const wchar_t* CSoundDecoder::GetLastErrorMessage()
{
	return lastErrorMessage.c_str();
}
