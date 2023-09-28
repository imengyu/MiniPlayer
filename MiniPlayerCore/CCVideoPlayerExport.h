﻿#pragma once
#include <stdint.h>

//播放器错误
//***************************************

#define VIDEO_PLAYER_ERROR_NOW_IS_LOADING 26
#define VIDEO_PLAYER_ERROR_ALREADY_OPEN 27
#define VIDEO_PLAYER_ERROR_STATE_CAN_ONLY_GET 28
#define VIDEO_PLAYER_ERROR_NOT_OPEN 29
#define VIDEO_PLAYER_ERROR_AV_ERROR 30 //FOR UI
#define VIDEO_PLAYER_ERROR_NO_VIDEO_STREAM 31 //FOR UI
#define VIDEO_PLAYER_ERROR_VIDEO_NOT_SUPPORT 32 //FOR UI
#define VIDEO_PLAYER_ERROR_NOR_INIT 33

//播放器事件
//***************************************
#define PLAYER_EVENT_OPEN_DONE 1
#define PLAYER_EVENT_CLOSED 2
#define PLAYER_EVENT_PLAY_DONE 3
#define PLAYER_EVENT_OPEN_FAIED 4
#define PLAYER_EVENT_INIT_DECODER_DONE 5

//解码器状态值
//***************************************

//用户可见的播放器状态值
enum class CCVideoState {
  Unknown = 0,
  Loading = 1,
  Failed = 2,
  NotOpen = 3,
  Playing = 4,
  Ended = 5,
  Opened = 6,
  Paused = 6,
};

//播放器初始化数据配置
//***************************************

/**
 * 播放器初始化数据
 */
class CCVideoPlayerInitParams {
public:
  /**
   * 同步队列最大大小
   */
  size_t MaxRenderQueueSize = 60;
  /**
   * 同步队列增长步长
   */
  size_t PacketPoolSize = 100;
  /**
   * 同步队列增长步长
   */
  size_t PacketPoolGrowStep = 10;

  /**
   * 目标格式
   * 
   * AVPixelFormat 默认 AV_PIX_FMT_RGBA
   */
  int DestFormat = 26;
  /**
   * 目标宽度
   */
  int DestWidth = 0;
  /**
   * 目标高度
   */
  int DestHeight = 0;
  /*
  * 解码器帧缓冲池大小
  */
  size_t FramePoolSize = 100;
  /*
  * 解码器帧缓冲池增长步长
  */
  size_t FramePoolGrowStep = 10;
  /*
  * 是否使用MediaCodec
  */
  bool UseMediaCodec = true;
  /**
   * 限制FPS
   */
  double LimitFps = 30;
};

//播放器状态值
//***************************************

class CCVideoPlayerAbstract;
//播放器事件回调
typedef void (*CCVideoPlayerEventCallback)(CCVideoPlayerAbstract* player, int message, void* customData);

//播放器实例
//***************************************
class CCVideoPlayerAbstract
{
public:
  CCVideoPlayerAbstract() {}
	virtual ~CCVideoPlayerAbstract() {}

  //初始配置和状态信息
  //**********************

  virtual CCVideoPlayerInitParams* GetInitParams() { return nullptr; }

  //播放器公共方法
  //**********************

  virtual bool OpenVideo(const char* filePath) { return false; }
  virtual bool CloseVideo() { return false; }

  virtual void SetVideoState(CCVideoState newState) {}
  virtual void SetVideoPos(int64_t pos) {}
  virtual void SetVideoVolume(int vol) {}

  virtual CCVideoState GetVideoState() { return CCVideoState::Unknown; }
  virtual int64_t GetVideoLength() { return 0; }
  virtual int64_t GetVideoPos() { return 0; }
  virtual int GetVideoVolume() { return 0; }
  virtual void GetVideoSize(int* w, int* h) {}

  //回调
  //**********************

  virtual void SetPlayerEventCallback(CCVideoPlayerEventCallback callback, void* data) {}
  virtual CCVideoPlayerEventCallback GetPlayerEventCallback() { return nullptr; }

  //错误处理
  //**********************

  virtual void SetLastError(int code, const wchar_t* errmsg) {}
  virtual void SetLastError(int code, const char* errmsg) {}
  virtual const wchar_t* GetLastErrorMessage() { return L""; }
  virtual int GetLastError() const { return 0; }
};




