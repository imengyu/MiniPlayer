#pragma once
#include "pch.h"
#include "CSoundDecoder.h"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

class COggDecoder :
	public CSoundDecoder
{
public:
	COggDecoder();
	~COggDecoder();

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

	TStreamFormat GetFormat() override { return TStreamFormat::sfOgg; }

	double GetCurrentPositionSecond();
	bool IsOpened() override;

	FILE * GetStream() {
		return _Stream;
	}
private:
	FILE * _Stream = NULL;
	OggVorbis_File _OggFile;
	double m_LengthSec = 0;
	ogg_int64_t m_CurSample = 0;
	ogg_int64_t m_Samples;
	int m_Channels;//声道数
	ULONG m_SampleRate;//采样率
	int m_BitsPerSample;
	int m_BitRate;
};

