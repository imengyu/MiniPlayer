#pragma once
#ifndef WITH_EXPORT_MODE
#include "CSoundDecoder.h"
#include "CSoundDevice.h"
#endif // WITH_EXPORT_MODE

//��Ƶ��ʽ��Ŀǰֻ֧�ּ��֣�
enum TStreamFormat
{
	sfUnknown = 0,
	sfMp3 = 1,//��֧��MP3
	sfOgg = 2,//��֧��OGG
	sfWav = 3,
	sfPCM = 4,
	sfFLAC = 5,//��֧��FLAC
	sfFLACOgg = 6,//��֧��FLAC  Vobis
	sfAC3 = 7,
	sfAacADTS = 8,
	sfWaveIn = 9,
	sfMidi = 10,
	sfWma = 11,
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

//������ʵ��
class CSoundPlayer
{

};


