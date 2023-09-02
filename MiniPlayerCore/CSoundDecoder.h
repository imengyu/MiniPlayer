#pragma once
#include "pch.h"
#include <string>

//解码器
class CSoundDecoder
{
public:
	CSoundDecoder();
	virtual ~CSoundDecoder() {}

	//打开文件
	virtual bool Open(LPWSTR file) { return false; }
	//关闭文件
	virtual bool Close() { return false; }
	//获取释放已经打开文件
	virtual bool IsOpened() { return false; }
	//释放
	virtual void Release() {
		delete this;
	}

	//获取音乐声道数
	virtual int GetChannelsCount() { return 0; }
	//获取音乐量化位数
	virtual int GetBitsPerSample() { return 0; }
	//获取音乐采样频率
	virtual ULONG GetSampleRate() { return 0; }
	//获取音乐时长（秒）
	virtual double GetLengthSecond() { return 0; }
	//获取音乐时长（sample）
	virtual DWORD GetLengthSample() { return 0; }
	//获取音乐比特率（没有完全支持）
	virtual int GetBitrate() { return 0; }

	//获取当前解码器解码位置（sample）
	virtual DWORD GetCurrentPositionSample() { return 0; }
	//获取当前解码器解码位置（秒）
	virtual double GetCurrentPositionSecond() { return 0; }
	//设置当前解码器解码位置（sample）
	virtual DWORD SeekToSample(DWORD sp) { return 0; }
	//设置当前解码器解码位置（秒）
	virtual double SeekToSecond(double sec) { return 0; }

	//解码并填充已解码的PCM数据至_Buffer中
	// * _Buffer 数据接收缓冲区
	// * _BufferSize 需要的数据大小，_Buffer大小必须>=_BufferSize
	// * 返回 已经填充的数据大小
	//    如果返回<_BufferSize或==0 ，GetLastErr()==L"" 说明已经播放完成
	virtual size_t Read(void*  _Buffer, size_t _BufferSize) { return 0; }

	//设置错误状态
	void SetLastError(int code, const wchar_t * errmsg);
	//设置错误状态
	void SetLastError(int code, const char* errmsg);
	//获取上次错误信息
	int GetLastError() { return lastErrorCode;  }
	//获取上次错误信息
	const wchar_t* GetLastErrorMessage();

private:
	int lastErrorCode = 0;
	std::wstring lastErrorMessage;
};

#define DECODER_ERROR_FILE_NOT_FOUND 20
#define DECODER_ERROR_OPEN_FILE_FAILED 21
#define DECODER_ERROR_BAD_FORMAT 22
#define DECODER_ERROR_BAD_HEADER 23
#define DECODER_ERROR_NOT_SUPPORT 24
#define DECODER_ERROR_INTERNAL_ERROR 25
#define DECODER_ERROR_NO_MEMORY 26