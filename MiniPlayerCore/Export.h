#pragma once

#ifdef MINI_PLAYER_LIB

#include "CSoundPlayer.h"
#include "CCVideoPlayer.h"
#define MINI_PLAYER_EXPORT __declspec(dllexport)

#else

#include "CSoundPlayerExport.h"
#include "CCVideoPlayerExport.h"
#define MINI_PLAYER_EXPORT 
#include "StringHelper.h"

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