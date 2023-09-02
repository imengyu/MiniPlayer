#pragma once

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
class CSoundPlayerBase
{
public:
	//加载文件到当前播放器
	//返回值：返回打开是否成功
	virtual bool Load(const wchar_t* path) { return false; }
	//关闭已加载文件
	//返回值：返回关闭是否成功
	virtual bool Close() { return false; }

	//开始或者继续播放
	//返回值：如果之前正在播放则返回false，否则返回true
	virtual bool Play() { return false; }
	//暂停播放
	//返回值：如果之前已暂停则返回false，否则返回true
	virtual bool Pause() { return false; }
	//停止播放（但没有关闭文件）
	virtual bool Stop() { return false; }
	//从头开始播放
	virtual bool Restart() { return false; }

	//获取当前播放进度
	//返回值：播放进度（单位秒）
	virtual double GetPosition() { return 0; }
	//设置当前播放进度
	//参数：
	//  * second 播放进度（单位秒）
	virtual void SetPosition(double second) {}

	//获取当前音乐时长
	//返回值：音乐时长（单位秒）
	virtual double GetDuration() { return 0; }
	/*获取播放器状态
	* 返回值：
	* TPlayerStatus::Unknow,
	* TPlayerStatus::NotOpen, 未打开文件
	* TPlayerStatus::Opened, 文件已打开就绪
	* TPlayerStatus::Playing, 正在播放
	* TPlayerStatus::Paused, 暂停中
	* TPlayerStatus::PlayEnd, 播放到音乐结尾了
	*/
	virtual TPlayerStatus GetState() { return TPlayerStatus::Unknow; }
	/*
	* 获取当前打开的文件格式
	* 返回值：TStreamFormat 文件格式
	*/
	virtual TStreamFormat GetFormat() { return TStreamFormat::sfUnknown; }

	//设置音量（0-100）
	virtual void SetVolume(int volume) {}
	//获取音量（0-100）
	virtual int GetVolume() { return 0; }

	//设置错误状态
	virtual void SetLastError(int code, const wchar_t* errmsg) {}
	//设置错误状态
	virtual void SetLastError(int code, const char* errmsg) {}
	//获取上次错误信息
	virtual int GetLastError() { return 0; }
	//获取上次错误信息
	virtual const wchar_t* GetLastErrorMessage() { return L""; }

	//获取当前音频比特率
	virtual unsigned long GetSampleRate() { return 0; }
	//获取当前音频采样位数(量化位数) （8/16/32）
	virtual int GetBitPerSample() { return 0; }
	//获取当前音频通道数
	virtual int GetChannelsCount() { return 0; }


	virtual unsigned int GetDurationSample() { return 0; }
	virtual unsigned int GetPositionSample() { return 0; }
	virtual void SetPosition(unsigned int second) {}
};

#define PLAYER_ERROR_NONE 0
#define PLAYER_ERROR_FILE_OPEN_FAILED 1
#define PLAYER_ERROR_DECODER_ERROR 2
#define PLAYER_ERROR_UNKNOWN_FILE_FORMAT 3
#define PLAYER_ERROR_NOT_SUPPORT_FORMAT 4



