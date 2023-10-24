#pragma once
#include "pch.h"
#include "CSoundDecoder.h"
#include "faad.h"

class CAacDecoder :
	public CSoundDecoder
{
public:
	CAacDecoder();
	~CAacDecoder();

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

	TStreamFormat GetFormat() override { return TStreamFormat::sfAacADTS; }

	size_t Read(void*  _Buffer, size_t _BufferSize) override;

	bool IsOpened() override;

private:
	ULONG sample_rate;
	int bit_per_sample;
	UCHAR channel; 
	
	DWORD all_samples = 0;
	DWORD cur_sample = 0;

	NeAACDecConfigurationPtr c_config = NULL;
	NeAACDecHandle hAac;
	NeAACDecFrameInfo frame_info;

	bool m_Opened = false;

	long _BufferSize = 0;
	BYTE* _PCMBuffer = NULL;
	BYTE* _AACBuffer = NULL;
	FILE*_Stream = NULL;

	void freebuffer();
};

int get_one_ADTS_frame(unsigned char ** buffer, size_t *buf_size, unsigned char * data, size_t * data_size);
int convert(NeAACDecHandle hAac, unsigned char ** bufferAAC, size_t *buf_sizeAAC, unsigned char ** bufferPCM, size_t  *buf_sizePCM, DWORD*allsamples);
