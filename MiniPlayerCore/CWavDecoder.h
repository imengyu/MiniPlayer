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
	char   			riffType[4];	//4byte,资源交换文件标志:RIFF	
	unsigned int   	riffSize;		//4byte,从下个地址到文件结尾的总字节数	
	char   			waveType[4];	//4byte,wav文件标志:WAVE	
	char   			formatType[4];	//4byte,波形文件标志:FMT(最后一位空格符)	
	unsigned int	formatSize;		//4byte,音频属性(compressionCode,numChannels,sampleRate,bytesPerSecond,blockAlign,bitsPerSample)所占字节数
	unsigned short	compressionCode;//2byte,格式种类(1-线性pcm-WAVE_FORMAT_PCM,WAVEFORMAT_ADPCM)
	unsigned short 	numChannels;	//2byte,通道数
	unsigned int   	sampleRate;		//4byte,采样率
	unsigned int   	bytesPerSecond;	//4byte,传输速率
	unsigned short 	blockAlign;		//2byte,数据块的对齐，即DATA数据块长度
	unsigned short 	bitsPerSample;	//2byte,采样精度-PCM位宽
	char   			dataType[4];	//4byte,数据标志:data
	unsigned int   	dataSize;		//4byte,从下个地址到文件结尾的总字节数，即除了wav header以外的pcm data length
}WAV_HEADER;
