#include "stdafx.h"
#include "CSoundOutPuter.h"
#include "CSoundPlayer.h"

CSoundOutPuter::CSoundOutPuter()
{
}
CSoundOutPuter::~CSoundOutPuter()
{
}

void CSoundOutPuter::SetOnCopyDataCallback(OnCopyDataFun callback)
{
	this->callback = callback;
}

void CSoundOutPuter::SetOnCheckEndCallback(OnCheckEndFun callback)
{
	checkEndCallback = callback;
}

bool CSoundOutPuter::OnCopyData(CSoundPlayer * instance, LPVOID buf, DWORD buf_len)
{
	return false;
}

DWORD CSoundOutPuter::OnCheckEnd(CSoundPlayer* instance)
{
	return 0;
}

bool CSoundOutPuter::Create(HWND hWnd, ULONG sample_rate, int channels, int bits_per_sample)
{
	return false;
}

bool CSoundOutPuter::Destroy()
{
	return false;
}

int CSoundOutPuter::GetVol()
{
	return 0;
}

void CSoundOutPuter::SetVol(int vol)
{
}

bool CSoundOutPuter::IsOutPuting()
{
	return false;
}

bool CSoundOutPuter::StartOutPut()
{
	return false;
}

bool CSoundOutPuter::EndOutPut()
{
	return false;
}

bool CSoundOutPuter::ResetOutPut()
{
	return false;
}
