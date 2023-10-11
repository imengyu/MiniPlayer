//
// Created by roger on 2020/12/22.
//

#ifndef VR720_CCPLAYERRENDER_H
#define VR720_CCPLAYERRENDER_H
#include "pch.h"
#include <thread>
extern "C" {
#include "libavutil/frame.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}
#include "CCPlayerDefine.h"
#include "CSoundDevice.h"
#include "CAppendBuffer.h"

class CCVideoDevice;
class CCVideoPlayerExternalData;
class CCPlayerRender : public CSoundDeviceHoster {
public:
  virtual bool Init(CCVideoPlayerExternalData* data);
  virtual void Destroy();
  virtual void Start(bool isStartBySeek);
  virtual void Reset();
  virtual void Stop();

  virtual CCVideoDevice* GetVideoDevice() { return videoDevice; }
  virtual CSoundDevice* GetAudioDevice() { return audioDevice; }
  virtual CCRenderState GetState() { return status; }

  virtual int64_t GetCurVideoDts() { return curVideoDts; }
  virtual int64_t GetCurVideoPts() { return curVideoPts; }
  virtual int64_t GetCurAudioDts() { return curAudioDts; }
  virtual int64_t GetCurAudioPts() { return curAudioPts; }

  virtual void SetVolume(int i);
  virtual int GetVolume() { return 0; }

  virtual CCVideoPlayerCallbackDeviceData* SyncRenderStart();
  virtual void SyncRenderEnd();

  bool GetShouldReSample() { return false; }
  void SetLastError(int code, const wchar_t* errmsg);

private:

  CCVideoDevice* CreateVideoDevice(CCVideoPlayerExternalData* data);
  CSoundDevice* CreateAudioDevice(CCVideoPlayerExternalData* data);

  CCVideoPlayerCallbackDeviceData renderDeviceData;

  SwrContext* swrContext = NULL;
  SwsContext* swsContext = nullptr;

  int64_t curVideoDts = 0;	//记录当前播放的视频流Packet的DTS
  int64_t curVideoPts = 0;	//记录当前播放的视频流Packet的DTS
  int64_t curAudioDts = 0;	//记录当前播放的音频流Packet的DTS
  int64_t curAudioPts = 0;	//记录当前播放的音频流Packet的DTS


  uint8_t** audioOutBuffer = nullptr; // 输出缓冲
  CAppendBuffer* audioOutLeave; // 输出遗留缓冲
  size_t destDataSize = 0;
  size_t destDataSizeOne = 0;//一个采样的数据大小（当前通道）
  uint32_t destNbSample = 0;//aac大小
  int destLinesize = 0;
  uint32_t destDataSizePerSample = 0;//一个采样的数据大小（单通道）
  uint32_t destLeaveSamples = 0;//拷贝剩余采样数量
  int destChannels = 0;

  //时钟
  double currentAudioClock = 0;
  double currentVideoClock = 0;


  //***************************************
  //状态控制

  CCVideoPlayerExternalData* externalData;

  CCVideoPlayerCallbackDeviceData syncRenderData;

  CCRenderState status = CCRenderState::NotRender;
  bool currentSeekToPosFinished = false;

  CSoundDevice* audioDevice = nullptr;
  CCVideoDevice* videoDevice = nullptr;

  AVFrame* currentFrame = nullptr;
  AVFrame* outFrame = nullptr;
  uint8_t* outFrameBuffer = nullptr;
  size_t outFrameBufferSize = 0;
  int outFrameDestWidth = 0;
  int outFrameDestHeight = 0;
  AVPixelFormat outFrameDestFormat = AV_PIX_FMT_RGBA;

  std::thread* renderVideoThread = nullptr;
  std::thread* renderAudioThread = nullptr;

  static void* RenderVideoThreadStub(void* param);
  void* RenderVideoThread();
  bool RenderVideoThreadWorker(bool sync);

  static bool RenderAudioBufferDataStub(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len, DWORD sample);
  bool RenderAudioBufferData(LPVOID buf, DWORD buf_len, DWORD sample);

  FILE* out_file;
};


#endif //VR720_CCPLAYERRENDER_H
