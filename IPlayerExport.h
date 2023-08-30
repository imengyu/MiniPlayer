#pragma once
#include "CSoundPlayer.h"
#include <string>

typedef struct {
	std::wstring* Title;
	std::wstring* Artist;
	std::wstring* Album;
	std::wstring* Time;
}MP3Info;

extern "C" bool __declspec(dllexport) GetMP3Info(LPWSTR file, MP3Info **outInfo, BOOL isWav);
extern "C" __declspec(dllexport) MP3Info*  GetMusicInfo(LPWSTR file, BOOL isWav);
extern "C" void __declspec(dllexport) DestroyMusicInfo(MP3Info* lastInfo);

//���� ������ʵ��
//*��ĳ��������ھ��
CSoundPlayer * CreateIPlayer(HWND hWnd);
//�ͷ� ������ʵ��
// * ���ڳ����˳�ʱʹ�ô˷���
// * iplayer ��������ʵ��
void DestroyIPlayer(CSoundPlayer*iplayer);

//�ر��Ѵ򿪵�����
extern "C" BOOL __declspec(dllexport) CloseMusic();
/*������
* file ������·��
*/
extern "C" BOOL __declspec(dllexport) OpenMusic(LPWSTR file);
/*ֹͣ����*/
extern "C" BOOL __declspec(dllexport) StopMusic();
/*
���Ż���ͣ����
* ����������ڲ��ţ�����ͣ���֣����������ͣ����ʼ�������֣��൱�ڲ���/��ͣ��ť����
*/
extern "C" BOOL __declspec(dllexport) PausePlayMusic();
/*��ʼ��������
* ���ñ����C����apiʱ�����ȳ�ʼ��������
* hWnd �������ߴ��ھ��
*/
extern "C" BOOL __declspec(dllexport) IPlayerInit(HWND hWnd);
/*��ͷ��ʼ���ŵ�ǰ����*/
extern "C" BOOL __declspec(dllexport) ReplayMusic();
/*��ʼ��������*/
extern "C" BOOL __declspec(dllexport) PlayMusic();
/*��ͣ���ŵ�ǰ����*/
extern "C" BOOL __declspec(dllexport) PauseMusic();
//��ȡ��ǰ���ֵĳ���
// * �� �� Ϊ��λ
extern "C" double __declspec(dllexport) GetMusicLength();
/*���õ�ǰ���ֲ���λ��
* val����ǰ���ֲ���λ�ã���λΪ ��
*/
extern "C" BOOL __declspec(dllexport) SetMusicPos(double val);
/*��ȡ��ǰ���ֲ���λ��
* ���أ���ǰ���ֲ���λ�ã���λΪ ��
*/
extern "C" double __declspec(dllexport) GetMusicPos();
/*��ȡ�����Ƿ��Ѵ�*/
extern "C" BOOL __declspec(dllexport) GetOpened();
/*��ȡ����ʱ���ַ���
* ��ʽ�ǣ�00:00 ����ʱ��
*/
extern "C" LPWSTR __declspec(dllexport) GetMusicLenTimeString();
/*��ȡ�������ڲ��ŵ�ʱ���ַ���
* ��ʽ�ǣ�00:00 ���ڲ���ʱ��
*/
extern "C" LPWSTR __declspec(dllexport) GetMusicPosTimeString();
/*��ȡ�������ڲ��ŵ�ʱ���ַ���
* ��ʽ�ǣ�00:00/00:00 ���ڲ���ʱ��/����ʱ��
*/
extern "C" LPWSTR __declspec(dllexport) GetMusicTimeString();
/*��ȡ��������һ��������Ϣ*/
extern "C" LPWSTR __declspec(dllexport) GetPlayerError();

//��ȡ���ֲ��ŵ�����λ��
// * �� ���� Ϊ��λ
// * ʹ�� GetMusicSampleRate ��ȡ ���ֲ�����
extern "C" DWORD __declspec(dllexport) GetMusicPosSample();
//�������ֲ��ŵ�λ��
// * �� ���� sample Ϊ��λ
extern "C" DWORD __declspec(dllexport) SetMusicPosSample(DWORD sample);

//��ȡ�����ļ��� ������
extern "C" DWORD __declspec(dllexport) GetMusicSampleRate();
//��ȡ�����ļ��� ����λ�� ��8/16/32��
extern "C" int __declspec(dllexport) GetMusicBitPerSample();
//��ȡ�����ļ��� ������
extern "C" int __declspec(dllexport) GetMusicChannelsCount();
//��ȡ�����ļ��� ����
// * �� ���� sample Ϊ��λ
extern "C" DWORD __declspec(dllexport) GetMusicLengthSample();

/*���ٲ�����
* ���ڳ����˳�ʱʹ�ô˷���
*/
extern "C" BOOL __declspec(dllexport) IPlayerDestroy();
/*���ֵ�����2s��
* currvol ����ǰ����
* �൱������������С
*/
extern "C" BOOL __declspec(dllexport) MusicTweedOut(int currvol);
/*���ֵ��루2s��
* currvol ����ǰ����
* �൱�������������
*/
extern "C" BOOL __declspec(dllexport) MusicTweedIn(int currvol);
/**/
extern "C" BOOL __declspec(dllexport) SetFFTHDC(HDC hdc);
/**/
extern "C" BOOL __declspec(dllexport) DrawFFTOnHDC(HDC hdc);
/**/
extern "C" BOOL __declspec(dllexport) DrawFFTOnHDCSmall(HDC hdc);
/*��ȡ������״̬
* TPlayerStatus::Unknow,
* TPlayerStatus::NotOpen, δ���ļ�
* TPlayerStatus::Opened, �ļ��Ѵ򿪾���
* TPlayerStatus::Playing, ���ڲ���
* TPlayerStatus::Paused, ��ͣ��
* TPlayerStatus::PlayEnd, ���ŵ����ֽ�β��
*/
extern "C" TPlayerStatus __declspec(dllexport) GetMusicPlayState();
/*��ȡ����������
* ����������ֵ ��1-100
*/
extern "C" int __declspec(dllexport) GetVolume();
/*���ò���������
* vol��������ֵ ��1-100��
*/
extern "C" BOOL __declspec(dllexport) SetVolume(int vol);
/*����Ĭ����Ƶ�������
* sample_rate ������
* channels ������
* bits_per_sample ����λ��
*/
extern "C" BOOL __declspec(dllexport) SetDefOutPutSettings(ULONG sample_rate, int channels, int bits_per_sample);
