#pragma once
#ifndef WITH_EXPORT_MODE
#include "CSoundDecoder.h"
#include "CSoundDevice.h"
#endif // WITH_EXPORT_MODE

//音频格式（目前只支持几种）
enum TStreamFormat
{
	sfUnknown = 0,
	sfMp3 = 1,//已支持MP3
	sfOgg = 2,//已支持OGG
	sfWav = 3,
	sfPCM = 4,
	sfFLAC = 5,//已支持FLAC
	sfFLACOgg = 6,//已支持FLAC  Vobis
	sfAC3 = 7,
	sfAacADTS = 8,
	sfWaveIn = 9,
	sfMidi = 10,
	sfWma = 11,
	sfM4a = 12,
	sfAutodetect = 1000//not used
};

//播放器状态
enum TPlayerStatus
{
	Unknow,
	//未打开文件
	NotOpen,
	//文件已打开就绪
	Opened,
	//正在播放
	Playing,
	//已暂停
	Paused,
	//播放到音乐结尾了
	PlayEnd,
};

//播放器实例
class CSoundPlayer
{

};


