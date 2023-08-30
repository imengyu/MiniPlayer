#pragma once
#include "stdafx.h"
#include "CSoundDecoder.h"

//��Ƶ��ʽ��Ŀǰֻ֧�ּ��֣�
enum TStreamFormat
{
	sfUnknown = 0,
	sfMp3 = 1,//��֧��MP3
	sfOgg = 2,//��֧��OGG
	sfWav = 3,//��֧��WAV
	sfPCM = 4,//��֧��PCM
	sfFLAC = 5,//��֧��FLAC
	sfFLACOgg = 6,//��֧��FLAC  Vobis
	sfAC3 = 7,
	sfAacADTS = 8,
	sfWaveIn = 9,//not used
	sfMidi = 10,//��֧��MIDI
	sfWma = 11,//��֧��wma
	sfM4a = 12,
	sfAutodetect = 1000//not used
};

//������״̬
enum TPlayerStatus
{
	Unknow,
	//δ���ļ�
	NotOpen,
	//�ļ��Ѵ򿪾���
	Opened,
	//���ڲ���
	Playing,
	//����ͣ
	Paused,
	//���ŵ����ֽ�β��
	PlayEnd,
};

class CDecoderPlugin
{
public:
	TStreamFormat TargetFormat = TStreamFormat::sfUnknown;
	LPWSTR DllPath;
	CSoundDecoder*Decoder = NULL;

};

typedef CSoundDecoder*(__cdecl*_CreateFun)(CDecoderPlugin*pu);
typedef BOOL(__cdecl*_DestroyFun)(CDecoderPlugin*pu, CSoundDecoder*de);

//������
class CSoundPlayer
{
public:
	CSoundPlayer();
	~CSoundPlayer();

	//���ؽ��������
	bool LoadDecoderPlugin(TStreamFormat format, LPWSTR path, LPCSTR createFun, LPCSTR destroyFun) { return false; };
	//ж�ؽ��������
	bool UnLoadDecoderPlugin(TStreamFormat format, LPWSTR path) { return false; };

	//������
	// *file ������·��
	virtual bool Open(LPWSTR file) { return false; }
	//�ر�����
	virtual bool Close() { return false; }
	/*
	���Ż���ͣ����
	* ����������ڲ��ţ�����ͣ���֣����������ͣ����ʼ�������֣��൱�ڲ���/��ͣ��ť����
	*/
	virtual bool PausePlay() { return false; }
	//��ʼ����
	virtual bool Play() { return false; }
	//��ͣ����
	virtual bool Pause() { return false; }
	//ֹͣ����
	virtual bool Stop() { return false; }
	//��ͷ��ʼ����
	virtual bool Restart() { return false; }

	//��ȡ��ǰ������
	virtual CSoundDecoder*GetCurrDecoder();

	//���ֵ���
	//* sec ��������Ҫ��ʱ��
	//* from ����ǰ����
	//* to����β����
	//* �൱������������С
	virtual void FadeOut(int sec, int from, int to) {}
	/*���ֵ���
	* sec ��������Ҫ��ʱ��
	* from ����ǰ����
	* to����β����
	* �൱�������������
	*/
	virtual void FadeIn(int sec, int from, int to) {}

	//��ȡ��ǰ�Ѵ����ֵĸ�ʽ
	virtual TStreamFormat GetFormat() { return (TStreamFormat)0; }
	//��ȡ������״̬
	virtual TPlayerStatus GetPlayerStatus() { return (TPlayerStatus)0; }

	//��ȡ��������һ��������Ϣ
	virtual LPWSTR GetLastErr();

	//��ȡ��ǰ���ֵĳ���
	// * �� sample Ϊ��λ
	virtual DWORD GetMusicLengthSample() { return 0; }
	//��ȡ���ֲ��ŵ�λ��
	// * �� �� Ϊ��λ
	virtual double GetMusicPos() { return 0; }
	//��ȡ��ǰ���ֵĳ���
	// * �� �� Ϊ��λ
	virtual double GetMusicLength() { return 0; }
	/*��ȡ�������ڲ��ŵ�ʱ���ַ���
	* ��ʽ�ǣ�00:00/00:00 ���ڲ���ʱ��/����ʱ��
	*/
	virtual LPWSTR GetMusicTimeString() { return 0; }
	/*��ȡ�������ڲ��ŵ�ʱ���ַ���
	* ��ʽ�ǣ�00:00 ���ڲ���ʱ��
	*/
	virtual LPWSTR GetMusicPosTimeString() { return 0; }
	/*��ȡ����ʱ���ַ���
	* ��ʽ�ǣ�00:00 ����ʱ��
	*/
	virtual LPWSTR GetMusicLenTimeString() { return 0; }

	//��ȡ���ֲ��ŵ�����λ��
	// * �� ���� Ϊ��λ
	// * ʹ�� GetMusicSampleRate ��ȡ ���ֲ�����
	virtual DWORD GetMusicPosSample() { return 0; }
	//�������ֲ��ŵ�λ��
	// * �� ���� sample Ϊ��λ
	virtual DWORD SetMusicPosSample(DWORD sample) { return 0; }

	//��ȡ�����ļ��� ������
	virtual DWORD GetMusicSampleRate() { return 0; }
	//��ȡ�����ļ��� ����λ�� ��8/16/32��
	virtual int GetMusicBitPerSample() { return 0; }
	//��ȡ�����ļ��� ������
	virtual int GetMusicChannelsCount() { return 0; }

	virtual void SetFFTHDC(HDC hdc) {}
	virtual void DrawFFTOnHDC(HDC hdc) {}
	virtual void DrawFFTOnHDCSmall(HDC hdc) {}
	//�������ֲ��ŵ�λ��
	// * �� �� Ϊ��λ
	virtual bool SetMusicPos(double sec) { return false; }
	//��ȡ�����Ƿ��Ѵ�
	virtual bool IsOpened() { return false; }
	/*��ȡ����������
	* ����������ֵ ��1-100
	*/
	virtual int GetPlayerVolume() { return 0; }
	/*���ò���������
	* vol��������ֵ ��1-100��
	*/
	virtual void SetPlayerVolume(int vol) {  }
	/*����Ĭ����Ƶ�������
	* sample_rate ������
	* channels ������
	* bits_per_sample ����λ��
	*/
	virtual void SetDefOutPutSettings(ULONG sample_rate, int channels, int bits_per_sample) {};

	//��ȡ�Ƿ����ڲ���midi����
	virtual bool IsPlayingMidi() { return false; };
	//���ô�����Ϣ
	// * errmsg��������Ϣ
	virtual bool err(LPWSTR errmsg) { return false; }
};

