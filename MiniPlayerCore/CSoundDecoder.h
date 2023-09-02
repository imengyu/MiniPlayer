#pragma once
#include "pch.h"
#include <string>

//������
class CSoundDecoder
{
public:
	CSoundDecoder();
	virtual ~CSoundDecoder() {}

	//���ļ�
	virtual bool Open(LPWSTR file) { return false; }
	//�ر��ļ�
	virtual bool Close() { return false; }
	//��ȡ�ͷ��Ѿ����ļ�
	virtual bool IsOpened() { return false; }
	//�ͷ�
	virtual void Release() {
		delete this;
	}

	//��ȡ����������
	virtual int GetChannelsCount() { return 0; }
	//��ȡ��������λ��
	virtual int GetBitsPerSample() { return 0; }
	//��ȡ���ֲ���Ƶ��
	virtual ULONG GetSampleRate() { return 0; }
	//��ȡ����ʱ�����룩
	virtual double GetLengthSecond() { return 0; }
	//��ȡ����ʱ����sample��
	virtual DWORD GetLengthSample() { return 0; }
	//��ȡ���ֱ����ʣ�û����ȫ֧�֣�
	virtual int GetBitrate() { return 0; }

	//��ȡ��ǰ����������λ�ã�sample��
	virtual DWORD GetCurrentPositionSample() { return 0; }
	//��ȡ��ǰ����������λ�ã��룩
	virtual double GetCurrentPositionSecond() { return 0; }
	//���õ�ǰ����������λ�ã�sample��
	virtual DWORD SeekToSample(DWORD sp) { return 0; }
	//���õ�ǰ����������λ�ã��룩
	virtual double SeekToSecond(double sec) { return 0; }

	//���벢����ѽ����PCM������_Buffer��
	// * _Buffer ���ݽ��ջ�����
	// * _BufferSize ��Ҫ�����ݴ�С��_Buffer��С����>=_BufferSize
	// * ���� �Ѿ��������ݴ�С
	//    �������<_BufferSize��==0 ��GetLastErr()==L"" ˵���Ѿ��������
	virtual size_t Read(void*  _Buffer, size_t _BufferSize) { return 0; }

	//���ô���״̬
	void SetLastError(int code, const wchar_t * errmsg);
	//���ô���״̬
	void SetLastError(int code, const char* errmsg);
	//��ȡ�ϴδ�����Ϣ
	int GetLastError() { return lastErrorCode;  }
	//��ȡ�ϴδ�����Ϣ
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