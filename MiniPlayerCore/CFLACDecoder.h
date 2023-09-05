#pragma once
#include "pch.h"
#include "CSoundDecoder.h"
#include "FLAC/stream_decoder.h"
#include "CSoundDevice.h"

class CFLACDecoder :
	public CSoundDecoder
{
public:
	CFLACDecoder(bool isOgg);
	~CFLACDecoder();

	static void CFLACDecoder_FLAC__StreamDecoderErrorCallback(const FLAC__StreamDecoder * decoder, FLAC__StreamDecoderErrorStatus status, void * data);
	static FLAC__StreamDecoderWriteStatus CFLACDecoder_FLAC__StreamDecoderWriteCallback(const FLAC__StreamDecoder * decoder, const FLAC__Frame * frame, const FLAC__int32 * const buffer[], void * data);
	static void CFLACDecoder_FLAC__StreamDecoderMetadataCallback(const FLAC__StreamDecoder * decoder, const FLAC__StreamMetadata * metadata, void * data);

	bool Open(const wchar_t* file) override;
	bool Close() override;


	int GetChannelsCount()override;
	int GetBitsPerSample()override;
	DWORD GetSampleRate()override;
	double GetLengthSecond()override;

	double SeekToSecond(double sec)override;
	double GetCurrentPositionSecond();

	DWORD GetLengthSample()override;
	DWORD GetCurrentPositionSample()override;
	DWORD SeekToSample(DWORD sp)override;

	size_t Read(void*  _Buffer, size_t _BufferSize) override;


	bool IsOpened() override;

	ULONG sample_rate;
	int bit_per_sample;
	int channel;

private:
	FLAC__StreamDecoder * pDecoder;//FLAC½âÂëÆ÷¶ÔÏó
	bool opened = false, isOgg = false;
	HANDLE hEventDecorderFinish = NULL;
	ULONG bfs = 0;
	UINT sps;

	BYTE buffer_cache2[BUFFERNOTIFYSIZE];
	BYTE buffer_cache[BUFFERNOTIFYSIZE*2];
	size_t readed_bytes = 0;
	FLAC__uint64 lengthSample;
	FLAC__uint64 curPos;
	double lengthSec;
	double curosSec;
};

