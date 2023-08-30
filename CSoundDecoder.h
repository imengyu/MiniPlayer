#pragma once
#include "stdafx.h"

//������
class CSoundDecoder
{
public:
	CSoundDecoder() {}
	virtual ~CSoundDecoder() {}

	//���ļ�
	virtual bool Open(LPWSTR file) { return false; }
	//�ر��ļ�
	virtual bool Close() { return false; }

	//��ȡ����������
	virtual int GetMusicChannelsCount() { return 0; }
	//��ȡ��������λ��
	virtual int GetMusicBitsPerSample() { return 0; }
	//��ȡ���ֲ���Ƶ��
	virtual ULONG GetMusicSampleRate() { return 0; }
	//��ȡ����ʱ�����룩
	virtual double GetMusicLength() { return 0; }
	//��ȡ����ʱ����sample��
	virtual DWORD GetMusicLengthSample() { return 0; }
	//��ȡ���ֱ����ʣ�û����ȫ֧�֣�
	virtual int GetMusicBitrate() { return 0; }

	//��ȡ��ǰ����������λ�ã�sample��
	virtual DWORD GetCurSample() { return 0; }
	//���õ�ǰ����������λ�ã�sample��
	virtual DWORD SeekToSample(DWORD sp) { return 0; }
	//��ȡ��ǰ����������λ�ã��룩
	virtual double GetCurSec() { return 0; }
	//���õ�ǰ����������λ�ã��룩
	virtual double SeekToSec(double sec) { return 0; }

	//���벢����ѽ����PCM������_Buffer��
	// * _Buffer ���ݽ��ջ�����
	// * _BufferSize ��Ҫ�����ݴ�С��_Buffer��С����>=_BufferSize
	// * ���� �Ѿ��������ݴ�С
	//    �������<_BufferSize��==0 ��GetLastErr()==L"" ˵���Ѿ��������
	virtual size_t Read(void*  _Buffer, size_t _BufferSize) { return 0; }

	//��ȡ�ͷ��Ѿ����ļ�
	virtual bool IsOpened() { return false; }

	//�ͷ�
	virtual void Release() { delete this; }

	//���ô���
	bool err(wchar_t const* errmsg);
	//���ô���
	bool err(char const* errmsg);

	//��ȡ�ϴδ���
	LPWSTR GetLastErr() {
		return lasterr;
	}

	bool CreateInPlugin = false;
private:
	WCHAR lasterr[256];

};

