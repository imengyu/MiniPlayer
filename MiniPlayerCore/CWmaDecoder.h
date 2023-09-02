#pragma once
#include "CSoundDecoder.h"
#include "CSoundDevice.h"
#include <wmsdk.h>
#include <wmsdkidl.h>
#include <io.h>

class CWmaDecoder :
	public CSoundDecoder
{
public:
	CWmaDecoder();
	~CWmaDecoder();

	bool Open(LPWSTR file) override;
	bool Close() override;

	int GetChannelsCount()override;
	int GetBitsPerSample()override;
	ULONG GetSampleRate()override;
	int GetMusicBitrate();

	size_t Read(void*  _Buffer, size_t _BufferSize) override;

	double GetLengthSecond()override;

	DWORD GetLengthSample()override;
	DWORD GetCurrentPositionSample()override;
	DWORD SeekToSample(DWORD sp)override;

	double SeekToSecond(double sec)override;
	double GetCurrentPositionSecond()override;

	bool IsOpened() override;

private:
	BOOL opened = 0;
	DWORD dwOffset = 0;;
	WORD wStream;
	BOOL bProtected;
	DWORD dwOutput;
	BOOL bHasAudio;
	DWORD dwSampleRate,
		dwChannels,
		dwBitsPerSample,
		dwBitrate = -1;

	IWMSyncReader * pWMSyncReader;
	INSSBuffer* pNSSBuffer;
	IWMHeaderInfo* pWMHeaderInfo;
	LARGE_INTEGER liDuration;
	LARGE_INTEGER liPos;

	BYTE lastoutbyte[BUFFERNOTIFYSIZE];
	size_t lastoutsize = 0;
};

