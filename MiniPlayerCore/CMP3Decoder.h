#pragma once
#include "pch.h"
#include "CSoundDecoder.h"
#include "mpg123.h"

class CMP3Decoder :
	public CSoundDecoder
{
public:
	CMP3Decoder();
	~CMP3Decoder();


	bool Open(const wchar_t* file) override;
	bool Close() override;

	int GetChannelsCount()override;
	int GetBitsPerSample()override;
	ULONG GetSampleRate()override;
	int GetBitrate();
	double GetLengthSecond()override;

	DWORD GetLengthSample()override;
	DWORD GetCurrentPositionSample()override;
	DWORD SeekToSample(DWORD sp)override;

	double SeekToSecond(double sec)override;
	size_t Read(void*  _Buffer, size_t _BufferSize) override;

	double GetCurrentPositionSecond();
	bool IsOpened() override;


private:
	mpg123_handle * _handle = NULL;
	off_t len, fpos = 0;
	LONG rate = 0;
	int bitrate = 0, channel = 2;
	int encoding = 0;
};