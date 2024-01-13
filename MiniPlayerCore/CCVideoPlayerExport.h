#pragma once
#include <stdint.h>
#include "CCAsyncTask.h"

//播放器错误
//***************************************

#define VIDEO_PLAYER_ERROR_NOW_IS_LOADING          26 //正在加载中，需要等待
#define VIDEO_PLAYER_ERROR_ALREADY_OPEN            27 //已经打开文件，无需重复打开
#define VIDEO_PLAYER_ERROR_STATE_CAN_ONLY_GET      28 //此状态只能获取不能设置
#define VIDEO_PLAYER_ERROR_NOT_OPEN                29 //播放器处于未打开状态
#define VIDEO_PLAYER_ERROR_AV_ERROR                30 //ffmpeg错误
#define VIDEO_PLAYER_ERROR_NO_VIDEO_STREAM         31 //当前文件无视频通道
#define VIDEO_PLAYER_ERROR_VIDEO_NOT_SUPPORT       32 //视频格式不支持
#define VIDEO_PLAYER_ERROR_NOR_INIT                33 //播放器未初始化
#define VIDEO_PLAYER_ERROR_RENDER_NOT_START        34 //渲染器还未开始，
#define VIDEO_PLAYER_ERROR_CAN_NOTCALL_THIS_TIME   35 //此时不能调用此方法

//播放器事件
//***************************************
#define PLAYER_EVENT_CLOSED               2 //文件关闭完成
#define PLAYER_EVENT_PLAY_END             3 //文件播放至末尾
#define PLAYER_EVENT_ASYNC_TASK           4 //异步事件回调
#define PLAYER_EVENT_INIT_DECODER_DONE    5 //初始化解码器完成
#define PLAYER_EVENT_RENDER_DATA_CALLBACK 6 //渲染回调，仅在 CCVideoPlayerInitParams.UseRenderCallback = true 时触发此事件。
#define PLAYER_EVENT_SEEK_DONE            7 //跳帧完成
#define PLAYER_EVENT_INIT_HW_DECODER_FAIL 8 //初始化硬件解码失败，转为软解


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
  Paused = 7,
  Closing = 8,
};

//异步任务数据
//***************************************

#define VIDEO_PLAYER_ASYNC_TASK_OPEN      0 //参数 const char*
#define VIDEO_PLAYER_ASYNC_TASK_CLOSE     1 //参数 nullptr
#define VIDEO_PLAYER_ASYNC_TASK_SET_STATE 2 //参数 int
#define VIDEO_PLAYER_ASYNC_TASK_GET_STATE 3 //参数 nullptr
#define VIDEO_PLAYER_ASYNC_TASK_SET_POS   4 //参数 int

struct CCVideoPlayerAsyncTask : public CCAsyncTask {
  std::string Path;
  CCVideoState State;
  int Pos;
};

//播放器初始化数据配置
//***************************************

/**
 * 播放器初始化数据
 */
class CCVideoPlayerInitParams {
public:
  /**
   * 帧队列最大大小
   */
  size_t MaxFrameQueueSize = 32;
  /**
   * 解码缓冲队列最小大小
   */
  size_t MinRenderQueueSize = 4;
  /**
   * 解码缓冲队列最大大小
   */
  size_t MaxRenderQueueSize = 8;
  /**
   * 解码缓冲队列增长步长
   */
  size_t RenderQueueSizeGrowStep = 32;
  /**
   * 包队列初始大小
   */
  size_t PacketPoolSize = 32;
  /**
   * 包队列增长步长
   */
  size_t PacketPoolGrowStep = 8;

  /**
   * 目标格式 (AVPixelFormat)
   */
  int DestFormat = 25;
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
  size_t FramePoolSize = 32;
  /*
  * 解码器帧缓冲池增长步长
  */
  size_t FramePoolGrowStep = 8;
  /*
  * 是否执行音视频同步
  */
  bool SyncVideoAndAudio = true;
  /*
   * 是否使用渲染数据回调
   */
  bool UseRenderCallback = true;
  /*
  * 同步运行
  */
  bool SyncRender = true;
  /*
  * 使用硬件加速
  */
  bool UseHadwareDecoder = true;
  /*
  * 输出音频设备
  */
  wchar_t OutAudioDevice[64];
  /**
   * 限制FPS
   */
  double LimitFps = 30;
};

//播放器状态值
//***************************************

class CCVideoPlayerAbstract;
//播放器事件回调
typedef void (*CCVideoPlayerEventCallback)(CCVideoPlayerAbstract* player, int message, void* eventData, void* customData);

//PLAYER_EVENT_RENDER_DATA_CALLBACK 事件
//***************************************

#define PLAYER_EVENT_RDC_TYPE_RENDER 0
#define PLAYER_EVENT_RDC_TYPE_PAUSE 1
#define PLAYER_EVENT_RDC_TYPE_REUSEME 2
#define PLAYER_EVENT_RDC_TYPE_RESET 3
#define PLAYER_EVENT_RDC_TYPE_DESTROY 4

//事件 PLAYER_EVENT_RENDER_DATA_CALLBACK 的参数定义
class CCVideoPlayerCallbackDeviceData {
public:
  int type;
  int width;
  int height;
  int64_t datasize;
  uint8_t** data;
  int* linesize;
  int64_t pts;
};

//播放器实例
//***************************************
class CCVideoPlayerAbstract
{
public:
  CCVideoPlayerAbstract() {}
	virtual ~CCVideoPlayerAbstract() {}

  //初始配置和状态信息
  //**********************

  /*
  * 获取初始配置
  */
  virtual CCVideoPlayerInitParams* GetInitParams() { return nullptr; }

  //播放器公共方法
  //**********************

  /*
  * 打开文件（同步）
  * 参数：
  *   * filePath：文件路径
  * 返回值：
  *   操作是否成功
  */
  virtual bool OpenVideo(const char* filePath) { return false; }
  /*
  * 打开文件（同步）
  * 参数：
  *   * filePath：文件路径
  * 返回值：
  *   操作是否成功
  */
  virtual bool OpenVideo(const wchar_t* filePath) { return false; }
  /*
  * 关闭文件（同步）
  * 返回值：
  *   操作是否成功
  */
  virtual bool CloseVideo() { return false; }

  /*
  * 设置是否启用推流模式。
  * 必须在打开文件之前调用。
  * 参数：
  *   * push：启用
  *   * type：推流类型 rtpm/rtsp
  *   * address：推流地址
  * 返回值：
  *   操作是否成功
  */
  virtual bool SetVideoPush(bool push, const char*, const char* address) { return false; }

  /*
  * 其他线程同步等待 OpenVideo 打开文件完成
  */
  virtual void WaitOpenVideo() {}
  /*
  * 其他线程同步等待 CloseVideo 关闭文件完成
  */
  virtual void WaitCloseVideo() {}

  /*
  * 设置播放器的状态，支持 暂停，继续，关闭
  * 参数：
  *   * newState：状态
  */
  virtual bool SetVideoState(CCVideoState newState) { return false; }
  /*
  * 设置播放器的播放位置（跳帧）。
  * 此操作只能跳至I帧。
  * 参数：
  *   * pos：位置，以毫秒为单位。
  */
  virtual bool SetVideoPos(int64_t pos) { return false; }
  /*
  * 设置播放器的音量
  * 参数：
  *   * vol：音量，0-100
  */
  virtual void SetVideoVolume(int vol) {}
  /*
  * 设置视频播放器是否循环播放
  * 参数：
  *   * loop：是否循环
  */
  virtual void SetVideoLoop(bool loop) {}

  /*
  * 获取播放器是否循环播放
  */
  virtual bool GetVideoLoop() { return false; }
  /*
  * 获取播放器是否启用推流模式
  */
  virtual bool GetVideoPush() { return false; }
  /*
  * 获取播放器当前状态
  */
  virtual CCVideoState GetVideoState() { return CCVideoState::Unknown; }
  /*
  * 获取播放器当前状态
  * 返回值：
  *   视频时长，以毫秒为单位
  */
  virtual int64_t GetVideoLength() { return 0; }
  /*
  * 获取播放器当前播放位置
  * 返回值：
  *   以毫秒为单位
  */
  virtual int64_t GetVideoPos() { return 0; }
  /*
  * 获取播放器的音量
  * 返回值：
  *   音量，0-100
  */
  virtual int GetVideoVolume() { return 0; }
  /*
  * 获取播放器的输出
  * 返回值：
  *   分贝值 int数组，2个分别是左/右声道
  */
  virtual int* GetCurrentOutPutDB() { return nullptr; }
  /*
  * 获取视频
  * 输出参数：
  *   * w: 视频的宽度
  *   * h: 视频的高度
  */
  virtual void GetVideoSize(int* w, int* h) {}

  //回调
  //**********************

  /*
  * 设置播放器事件回调
  * 事件值：参见下方 PLAYER_EVENT_
  */
  virtual void SetPlayerEventCallback(CCVideoPlayerEventCallback callback, void* data) {}
  /*
  * 获取播放器事件回调
  */
  virtual CCVideoPlayerEventCallback GetPlayerEventCallback() { return nullptr; }

  //同步渲染
  //**********************

  /*
  * 手动触发同步渲染，仅在 CCVideoPlayerInitParams.SyncRender = true 时有效。
  */
  virtual CCVideoPlayerCallbackDeviceData* SyncRenderStart() { return nullptr; }
  /*
  * 同步渲染结束
  */
  virtual void SyncRenderEnd() {}
  /*
  * 手动丢包
  */
  virtual void SyncRenderDropPack(int count) {}

  /*
  * 渲染器更新输出缓冲区大小
  */
  virtual void RenderUpdateDestSize(int width, int height) {}

  //错误处理
  //**********************

  virtual void SetLastError(int code, const wchar_t* errmsg) {}
  virtual void SetLastError(int code, const char* errmsg) {}
  //获取错误信息
  virtual const wchar_t* GetLastErrorMessage() { return L""; }
  //获取错误信息(utf8编码)
  virtual const char* GetLastErrorMessageUtf8() { return ""; }
  //获取错误号
  //返回值：参见上方：VIDEO_PLAYER_ERROR_*
  virtual int GetLastError() const { return 0; }

  //执行播放器异步调用
  //参数：
  //  * command 命令ID，具体参照 VIDEO_PLAYER_ASYNC_TASK_* 的命令
  //  * data  命令参数，每个命令有不同参数
  //返回值：当前命令ID
  //命令完成消息将由 PLAYER_EVENT_ASYNC_TASK 事件通知
  virtual int PostWorkerThreadCommand(int command, void* data) { return -1; }
};




