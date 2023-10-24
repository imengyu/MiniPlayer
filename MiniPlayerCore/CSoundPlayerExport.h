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
	//正在加载中
	Loading,
};

class CSoundPlayer;

//播放器事件回调
typedef void (*CSoundPlayerEventCallback)(CSoundPlayer* player, int message, void* customData);

//播放器实例
class CSoundPlayer
{
public:
	CSoundPlayer() {}
	virtual ~CSoundPlayer() {}

	//加载文件到当前播放器
	//返回值：返回打开是否成功
	virtual bool Load(const wchar_t* path) { return false; }
	//预加载文件到当前播放器。
	// * 预加载完成后直接调用Play即可播放。
	// * 或者当前正在播放的文件达到末尾后，立即自动接上播放已预加的内容
	// * 如果播放器处于未加载状态，则直接调用正常的加载方法，无需预加载
	// * 最多预加载1个内容，重复调用此方法会导致之前预加载内容失效
	//返回值：返回打开是否成功
	virtual bool PreLoad(const wchar_t* path) { return false; }
	//关闭已加载文件
	//返回值：返回关闭是否成功
	virtual bool Close() { return false; }

	//获取当前是否有预加载内容
	//返回值：返回当前是否有预加载内容
	virtual bool IsPreLoad() { return false; }

	//设置事件通知回调。回调在非主线程触发
	virtual void SetEventCallback(CSoundPlayerEventCallback callback, void* customData) {}

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

	//设置音量
	//参数：
	//  * volume 音量（0.0-1.0）
	//  * index 设置音量的通道，例如如果是双通道声音，则0为左声道，1为右声道。-1为设置所有声道音量
	virtual void SetVolume(float volume, int index = 0) {}
	//获取音量
	//参数：
	//  * index 获取音量的通道，例如如果是双通道声音，则0为左声道，1为右声道
	//返回值：音量（0.0-1.0）
	virtual float GetVolume(int index = 0) { return 0;  }

	//设置错误状态
	virtual void SetLastError(int code, const wchar_t* errmsg) {}
	//设置错误状态
	virtual void SetLastError(int code, const char* errmsg) {}
	//获取上次错误码
	//返回值：参见下方 PLAYER_ERROR_*
	virtual int GetLastError() { return 0; }
	//获取上次错误信息
	virtual const wchar_t* GetLastErrorMessage() { return L""; }
	//获取上次错误信息
	virtual const char* GetLastErrorMessageUtf8() { return ""; }

	//获取当前音频比特率
	virtual unsigned long GetSampleRate() { return 0; }
	//获取当前音频采样位数(量化位数) （8/16/32）
	virtual int GetBitPerSample() { return 0; }
	//获取当前音频通道数
	virtual int GetChannelsCount() { return 0; }


	virtual unsigned int GetDurationSample() { return 0; }
	virtual unsigned int GetPositionSample() { return 0; }	
	virtual void SetPositionSample(unsigned int sample) {}
};

#define PLAYER_ERROR_NONE                0 //无错误
#define PLAYER_ERROR_FILE_OPEN_FAILED    1 //文件打开错误
#define PLAYER_ERROR_DECODER_ERROR       2 //解码器错误
#define PLAYER_ERROR_UNKNOWN_FILE_FORMAT 3 //未知文件格式
#define PLAYER_ERROR_NOT_SUPPORT_FORMAT  4 //不支持的格式
#define PLAYER_ERROR_OUTPUT_ERROR        5 //输出音频设备错误
#define PLAYER_ERROR_NOT_LOAD            6 //未加载成功，请稍等或者检查打开状态
#define PLAYER_ERROR_DEVICE_INVALID      7 //音频输出设备被拔出或者不可用
#define PLAYER_ERROR_SERVICE_NOT_RUN     8 //Windows 音频服务未运行。(AUDCLNT_E_SERVICE_NOT_RUNNING)
#define PLAYER_ERROR_LOADING             9 //有操作正在执行，不能重复请求

#define SOUND_PLAYER_EVENT_PLAY_END      1 //播放到末尾
#define SOUND_PLAYER_EVENT_PLAY_ERROR    2 //播放中途错误


