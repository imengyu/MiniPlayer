#pragma once

#ifdef MINI_PLAYER_LIB

#include "CSoundPlayer.h"
#include "CCVideoPlayer.h"
#define MINI_PLAYER_EXPORT __declspec(dllexport)

#else

#include "CSoundPlayerExport.h"
#include "CCVideoPlayerExport.h"
#include "CCAsyncTask.h"

#ifdef MINI_PLAYER_EXE

#include "CCEvent.h"
#include "CCSimpleStack.h"
#include "CCSimpleQueue.h"
#include "StringHelper.h"
#include "Logger.h"

#endif

#endif

//
// * 说明：
//   创建视频播放器实例
//
// * 参数：
//   param: 初始化参数
// 
// * 返回：
//   播放器实例指针
//
extern "C" MINI_PLAYER_EXPORT CCVideoPlayerAbstract * CreateVideoPlayer(CCVideoPlayerInitParams * param);

//
// * 说明：
//   销毁播放器实例
//
// * 参数：
//   * player 播放器实例指针
// 
// * 返回：
//   无
//
extern "C" MINI_PLAYER_EXPORT void DestroyVideoPlayer(CCVideoPlayerAbstract * player);

//
// * 说明：
//   创建音频播放器实例
//
// * 参数：
//   无
// 
// * 返回：
//   播放器实例指针
//
extern "C" MINI_PLAYER_EXPORT CSoundPlayer * CreateSoundPlayer();

//
// * 说明：
//   销毁播放器实例
//
// * 参数：
//   * player 播放器实例指针
// 
// * 返回：
//   无
//
extern "C" MINI_PLAYER_EXPORT void DestroySoundPlayer(CSoundPlayer * player);

//
// * 说明：
//   获取音频文件的时长
//
// * 参数：
//   * file ：文件路径
// 
// * 返回：
//   音频时长（秒）
//
extern "C" MINI_PLAYER_EXPORT double GetAudioDurationFast(const wchar_t* file);

//
// * 说明：
//   获取音频文件的格式
//
// * 参数：
//   * file ：文件路径
// 
// * 返回：
//   格式枚举
//
extern "C" MINI_PLAYER_EXPORT TStreamFormat GetAudioFileFormat(const wchar_t* file);

//
// * 说明：
//   获取音频输出设备列表
//
// * 参数：
//   * [输出] outList ：一个指针变量地址，用于存放返回的设备列表信息。
// 
// * 返回：
//   操作是否成功
//
extern "C" MINI_PLAYER_EXPORT bool GetAllAudioOutDeviceInfo(CSoundDeviceAudioOutDeviceInfo **outList, int* outCount);

//
// * 说明：
//   获取音频文件的格式
//
// * 参数：
//   * ptr ：由 GetAllAudioOutDeviceInfo 函数返回的列表信息地址
// 
// * 返回：
//   无
//
extern "C" MINI_PLAYER_EXPORT void DeleteAllAudioOutDeviceInfo(CSoundDeviceAudioOutDeviceInfo **ptr);

//读取视频信息
struct READ_VIDEO_INFO {
  //指定获取信息是否成功
  bool success;
  //如果 success 为 false，则这个字段说明错误信息
  wchar_t lastError[256];
  //当前视频的格式信息
  char format[100];
  //当前视频的时长（秒）
  double duration;
  //当前视频的宽度
  int width;
  //当前视频的高度
  int height;
  //当前视频的帧率
  double frameRate;
  //当前视频的帧率
  double avgFrameRate;
  //当前主音频的采样率
  double simpleRate;
  //指示是否是视频
  bool isVideo;
  //指示是否是音频
  bool isAudio;
};

struct CM_API_RESULT {
  void* Result;
  char ErrorMessage[64];
  bool Success;
};

//
// * 说明：
//   释放视频格式信息结构体
//
// * 参数：
//   * ptr ：格式信息结构体
// 
// * 返回：
//   无
//
extern "C" MINI_PLAYER_EXPORT void ReleaseVideoInfo(READ_VIDEO_INFO * ptr);

//
// * 说明：
//   获取视频文件的格式信息
//
// * 参数：
//   * pchFileName ：文件路径
// 
// * 返回：
//   格式信息结构体，不需要使用时需要调用 ReleaseVideoInfo 函数释放
//
extern "C" MINI_PLAYER_EXPORT READ_VIDEO_INFO * GetVideoInfo(const wchar_t* pchFileName);

//
// * 说明：
//   释放本模块暴露出去的内存地址
//
// * 参数：
//   * ptr ：内存指针
// 
// * 返回：
//   无
//
extern "C" MINI_PLAYER_EXPORT void DeletePlayerMemory(void* ptr);


//
// * 说明：
//   浮点数组的PCM数据转码为wav文件
//
// * 参数：
//   * output_file ：生成文件路径
//   * pcm_data ：数据
//   * pcm_count ：数据长度
//   * sample_rate ：采样率
//   * channels ：声音通道数
// 
// * 返回：
//   操作是否成功以及返回值
//
extern "C" MINI_PLAYER_EXPORT CM_API_RESULT* FloatPCMArrayToWavFile(const char* output_file, float* pcm_data, long pcm_count, int sample_rate, int channels);