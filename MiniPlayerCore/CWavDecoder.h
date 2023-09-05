#pragma once
#include "CSoundDecoder.h"
#include <MMSystem.h>

class CWavDecoder :
	public CSoundDecoder
{
public:
	CWavDecoder();
	~CWavDecoder();

	bool Open(const wchar_t* file) override;
	bool Close() override;

	int GetChannelsCount()override;
	int GetBitsPerSample()override;
	ULONG GetSampleRate()override;
	double GetLengthSecond()override;

	DWORD GetLengthSample()override;
	DWORD GetCurrentPositionSample()override;
	DWORD SeekToSample(DWORD sp)override;

	double SeekToSecond(double sec)override;
	size_t Read(void*  _Buffer, size_t _BufferSize) override;

	int Seek(long _Offset, int _Origin);

	double GetCurrentPositionSecond();
	bool IsOpened() override;

private:
	HMMIO hStream=NULL;
	MMIOINFO wavInfo;

	double _BitPerSample = 0;
	int _DataOffset = 0;
	int _FileSize = 0;
	double _FileSec = 0;
	double _CurSec = 0;
	long cur = 0;

	MMCKINFO ckIn;
	WAVEFORMATEX m_pwfx;
	MMCKINFO m_ckRiff;
};

/* RIFF WAVE file struct.
	* For details see WAVE file format documentation
	* (for example at http://www.wotsit.org).
	*/
typedef struct WAV_HEADER_S
{
	char   			riffType[4];	//4byte,��Դ�����ļ���־:RIFF	
	unsigned int   	riffSize;		//4byte,���¸���ַ���ļ���β�����ֽ���	
	char   			waveType[4];	//4byte,wav�ļ���־:WAVE	
	char   			formatType[4];	//4byte,�����ļ���־:FMT(���һλ�ո��)	
	unsigned int	formatSize;		//4byte,��Ƶ����(compressionCode,numChannels,sampleRate,bytesPerSecond,blockAlign,bitsPerSample)��ռ�ֽ���
	unsigned short	compressionCode;//2byte,��ʽ����(1-����pcm-WAVE_FORMAT_PCM,WAVEFORMAT_ADPCM)
	unsigned short 	numChannels;	//2byte,ͨ����
	unsigned int   	sampleRate;		//4byte,������
	unsigned int   	bytesPerSecond;	//4byte,��������
	unsigned short 	blockAlign;		//2byte,���ݿ�Ķ��룬��DATA���ݿ鳤��
	unsigned short 	bitsPerSample;	//2byte,��������-PCMλ��
	char   			dataType[4];	//4byte,���ݱ�־:data
	unsigned int   	dataSize;		//4byte,���¸���ַ���ļ���β�����ֽ�����������wav header�����pcm data length
}WAV_HEADER;
