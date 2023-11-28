//
// Created by roger on 2020/12/19.
//

#include "pch.h"
#include "CCVideoPlayer.h"
#include "CCPlayerDefine.h"
#include "StringHelper.h"
#include "Logger.h"

CCVideoPlayer::CCVideoPlayer() {
  Init(nullptr);
}
CCVideoPlayer::CCVideoPlayer(CCVideoPlayerInitParams* initParams) {
  Init(initParams);
}
CCVideoPlayer::~CCVideoPlayer() {
  Destroy();
}

void CCVideoPlayer::PostPlayerEventCallback(int message, void* data) {
  auto task = new CCVideoPlayerPostEventAsyncTask();
  task->event = message;
  task->eventDataData = data;
  postBackQueue.Push(task);
}
void CCVideoPlayer::CallPlayerEventCallback(int message, void* data) {
  if (videoPlayerEventCallback != nullptr)
    videoPlayerEventCallback(this, message, data, videoPlayerEventCallbackData);
}
int CCVideoPlayer::GetLastError() const { return lastErrorCode; }

//播放器子线程方法
//**************************

void CCVideoPlayer::DoSetVideoState(CCVideoState state) {
  setVideoStateLock.lock();
  videoState = state;
  setVideoStateLock.unlock();
}
std::string CCVideoPlayer::GetAvError(int code)
{
  char errBuf[128];
  if (av_strerror(code, errBuf, sizeof(errBuf)) == 0)
    return StringHelper::FormatString("%s (%d)", errBuf, code);
  return StringHelper::FormatString("Unknow code: %d", code);
}

//播放器公共方法
//**************************

bool CCVideoPlayer::OpenVideo(const char* filePath) {

  LOGDF("OpenVideo: Call. %hs", StringHelper::Utf8ToUnicode(filePath).c_str());

  if (videoState == CCVideoState::Loading) {
    SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "OpenVideo: Player is loading, please wait a second");
    return false;
  }  
  if (videoState == CCVideoState::Closing)
    WaitCloseVideo();
  if (videoState > CCVideoState::NotOpen)
    CloseVideo();

  currentFile = filePath;
  DoSetVideoState(CCVideoState::Loading);

  openDoneEvent.Reset();

  LOGD("DoOpenVideo: Start");

  if (!InitDecoder()) {
    DoSetVideoState(CCVideoState::Failed);
    LOGD("DoOpenVideo: InitDecoder failed");
    return false;
  }

  PostPlayerEventCallback(PLAYER_EVENT_INIT_DECODER_DONE);
  LOGD("DoOpenVideo: InitDecoder done");

  decodeQueue.Reset();

  if (!pushMode && !render->Init(&externalData)) {
    DoSetVideoState(CCVideoState::Failed);
    LOGD("DoOpenVideo: Init render failed");
    return false;
  }

  DoSetVideoState(CCVideoState::Opened);
  openDoneEvent.NotifyOne();
  LOGD("DoOpenVideo: Done");

  return true;
}
bool CCVideoPlayer::OpenVideo(const wchar_t* filePath) {
  return OpenVideo(StringHelper::UnicodeToUtf8(filePath).c_str());
}
bool CCVideoPlayer::CloseVideo() {
  LOGD("CloseVideo: Call");

  if (videoState == CCVideoState::Closing)
    return true;
  if (videoState == CCVideoState::NotOpen || videoState == CCVideoState::Failed) {
    SetLastError(VIDEO_PLAYER_ERROR_NOT_OPEN, "Can not close video because video not load");
    return true;
  }

  DoSetVideoState(CCVideoState::Closing);

  LOGD("DoCloseVideo: Start");

  closeDoneEvent.Reset();

  //停止
  StopAll();

  //释放
  DestroyDecoder();
  render->Destroy();

  DoSetVideoState(CCVideoState::NotOpen);
  closeDoneEvent.NotifyOne();

  PostPlayerEventCallback(PLAYER_EVENT_CLOSED);
  LOGD("DoCloseVideo: Done");

  return true;
}
void CCVideoPlayer::WaitOpenVideo() {
  openDoneEvent.Wait();
}
void CCVideoPlayer::WaitCloseVideo() {
  closeDoneEvent.Wait();
}
bool CCVideoPlayer::SetVideoState(CCVideoState newState) {

  if (setVideoStateLock.try_lock())
    setVideoStateLock.unlock();
  if (videoState == newState)
    return true;

  LOGDF("SetVideoState : %s", CCVideoStateToString(newState));

  if (videoState == CCVideoState::Loading) {
    SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "SetVideoState: Now player is loading");
    return false;
  }
  if (videoState == CCVideoState::Closing) {
    SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "SetVideoState: Now player is closing");
    return false;
  }


  switch (newState) {
  case CCVideoState::Playing: {
    DoSetVideoState(CCVideoState::Loading);
    StartAll();
    DoSetVideoState(CCVideoState::Playing);
    break;
  }
  case CCVideoState::Paused: {
    DoSetVideoState(CCVideoState::Loading);
    StopAll();
    DoSetVideoState(CCVideoState::Paused);
    break;
  }
  default:
    SetLastError(VIDEO_PLAYER_ERROR_STATE_CAN_ONLY_GET, 
      StringHelper::FormatString("Bad state %s, this state can only get.", CCVideoStateToString(newState)).c_str()
    );
    return false;
  }

  return true;
}
bool CCVideoPlayer::SetVideoPos(int64_t pos) {

  if (videoState == CCVideoState::Loading) {
    SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "SetVideoPos: Now player is loading");
    return false;
  }
  if (videoState == CCVideoState::Closing) {
    SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "SetVideoPos: Now player is closing");
    return false;
  }
  if (videoState == CCVideoState::NotOpen) {
    SetLastError(VIDEO_PLAYER_ERROR_NOT_OPEN, "SetVideoPos: Can not seek video because video not load");
    return false;
  }

  playerSeeking = true;

  seekDest = externalData.StartTime + pos;

  if (seekDest == render->GetCurVideoPts()) {
    PostPlayerEventCallback(PLAYER_EVENT_SEEK_DONE);
    LOGD("Seek: Seek done");
    return true;
  }

  if (videoState == CCVideoState::Playing) {
    LOGD("Seek: Stop before seek");

    //先停止序列
    StopDecoderThread();
    render->Stop();
  }

  LOGDF("Seek: Seek to %d", seekDest);

  //跳转到指定帧

  int ret;

  if (videoIndex != -1) {

    seekPosVideo = av_rescale_q(seekDest * 1000, av_get_time_base_q(), formatContext->streams[videoIndex]->time_base);
    if (seekPosVideo > render->GetCurVideoPts())
      ret = av_seek_frame(formatContext, videoIndex, seekPosVideo, AVSEEK_FLAG_FRAME);
    else
      ret = av_seek_frame(formatContext, videoIndex, seekPosVideo, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    if (ret != 0) {
      LOGEF("av_seek_frame video failed : %s", GetAvError(ret).c_str());
      SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("av_seek_frame video failed : %s", GetAvError(ret).c_str()).c_str());
      playerSeeking = false;
      return false;
    }
    avcodec_flush_buffers(videoCodecContext);//清空缓存数据
  }

  if(audioIndex != -1) {
      seekPosAudio = av_rescale_q(seekDest * 1000, av_get_time_base_q(), formatContext->streams[audioIndex]->time_base);
      ret = av_seek_frame(formatContext, audioIndex, seekPosAudio, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_ANY);
      if (ret != 0)
          LOGEF("av_seek_frame audio failed : %s", GetAvError(ret).c_str());
      else
          avcodec_flush_buffers(audioCodecContext);//清空缓存数据
  }

  //清空队列中所有残留数据
  decodeQueue.ClearAll();

  decoderAudioFinish = audioIndex == -1;
  decoderVideoFinish = false;

  if (videoState == CCVideoState::Playing) {
    //启动线程
    LOGD("Seek: Start all for seek");
    StartDecoderThread();
    render->Start();
  }

  PostPlayerEventCallback(PLAYER_EVENT_SEEK_DONE);
  LOGD("Seek: Seek done"); 
  
  playerSeeking = false;
  return true;
}
int64_t CCVideoPlayer::GetVideoPos() {

  if (videoState <= CCVideoState::NotOpen) {
    SetLastError(VIDEO_PLAYER_ERROR_NOT_OPEN, "");
    return -1;
  }

  if (playerSeeking == 1)
    return seekDest - externalData.StartTime;

  if (!render || !formatContext)
    return -1;

  if (videoIndex != -1)
    return (int64_t)(render->GetCurVideoPts() * av_q2d(formatContext->streams[videoIndex]->time_base) * 1000);
  if (audioIndex != -1)
    return (int64_t)(render->GetCurAudioPts() * av_q2d(formatContext->streams[audioIndex]->time_base) * 1000);

  return -1;
}
bool CCVideoPlayer::GetVideoPush() { return pushMode; }
bool CCVideoPlayer::GetVideoLoop() { return loop; }
CCVideoState CCVideoPlayer::GetVideoState() { 
  if (setVideoStateLock.try_lock())
    setVideoStateLock.unlock();
  return videoState;
}
int64_t CCVideoPlayer::GetVideoLength() {
  if (!formatContext) {
    SetLastError(VIDEO_PLAYER_ERROR_NOT_OPEN, "Video not open");
    return 0;
  }
  return (int64_t)(formatContext->duration / (double)AV_TIME_BASE * (double)1000); //ms
}
void CCVideoPlayer::SetVideoVolume(int vol) { render->SetVolume(vol); }
void CCVideoPlayer::SetVideoLoop(bool loop)
{
  this->loop = loop;
}
bool CCVideoPlayer::SetVideoPush(bool push, const char* type, const char* address)
{
  if (videoState != CCVideoState::NotOpen) {
    SetLastError(VIDEO_PLAYER_ERROR_CAN_NOTCALL_THIS_TIME, "SetVideoPush: Can only call when player not open");
    return false;
  }

  pushMode = push;
  pushAddress = address;
  pushType = type;
  return true;
}
int CCVideoPlayer::GetVideoVolume() { return render->GetVolume(); }
void CCVideoPlayer::GetVideoSize(int* w, int* h) {
  if (formatContext) {
    *w = formatContext->streams[videoIndex]->codecpar->width;
    *h = formatContext->streams[videoIndex]->codecpar->height;
  }
}

void CCVideoPlayer::StartAll() {
  if (videoState == CCVideoState::Playing)
    return;
  DoSetVideoState(CCVideoState::Playing);

  decoderAudioFinish = audioIndex == -1;
  decoderVideoFinish = false;
  StartDecoderThread();

  if (!pushMode)
    render->Start();
}
void CCVideoPlayer::StopAll() {
  if (videoState == CCVideoState::Paused || videoState == CCVideoState::Ended)
    return;
  DoSetVideoState(CCVideoState::Paused);
  StopDecoderThread();
  if (!pushMode)
    render->Stop();
}

//解码器初始化与反初始化
//**************************

int CCVideoPlayer::InitHwDecoder(AVCodecContext* ctx, const enum AVHWDeviceType type)
{
  int err = 0;
  //创建硬件设备信息上下文 
  if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0)) < 0) {
    LOGE("InitHwDecoder: Failed to create specified HW device.");
    return err;
  }
  //绑定编解码器上下文和硬件设备信息上下文
  ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
  return err;
}
AVPixelFormat CCVideoPlayer::GetHwFormat(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
  const enum AVPixelFormat* p;

  for (p = pix_fmts; *p != -1; p++) {
    if (*p == ((CCVideoPlayer*)ctx->opaque)->hw_pix_fmt)
      return *p;
  }

  LOGE("InitHwDecoder: Failed to get HW surface format");
  return AV_PIX_FMT_NONE;
}

bool CCVideoPlayer::InitDecoder() {

  int ret;
  AVCodecParameters* codecParameters = nullptr;
  enum AVHWDeviceType type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;

  decodeState = CCDecodeState::Preparing;

  //寻找硬件解码类型
  if (InitParams.UseHadwareDecoder)
    type = AVHWDeviceType::AV_HWDEVICE_TYPE_CUDA;

  formatContext = avformat_alloc_context();
  //打开视频数据源
  int openState = avformat_open_input(&formatContext, currentFile.c_str(), nullptr, nullptr);
  if (openState < 0) {
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("Failed to open input file, error : %s", GetAvError(openState).c_str()).c_str());
    goto INIT_FAIL_CLEAN;
  }
  //为分配的AVFormatContext 结构体中填充数据
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, "Failed to read the input video stream information");
    goto INIT_FAIL_CLEAN;
  }

  videoIndex = -1;
  audioIndex = -1;

  //LOGD("---------------- File Information ---------------");
  //av_dump_format(formatContext, 0, currentFile.c_str(), 0);
  //LOGD("-------------- File Information end -------------");

  //找到视频流，音频流
  //***********************************

  ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (ret < 0)
    videoIndex = -1;
  else {
    videoIndex = ret;
    LOGDF("InitDecoder: video stream : %d", ret);
  }
  ret = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (ret < 0)
    audioIndex = -1;
  else {
    audioIndex = ret;
    LOGDF("InitDecoder: audio stream : %d", ret);
  }

  LOGDF("InitDecoder: streams count : %d", formatContext->nb_streams);
  LOGDF("InitDecoder: audioIndex : %d", audioIndex);
  LOGDF("InitDecoder: videoIndex : %d", videoIndex);

  //无视频流
  if (videoIndex != -1) {
    //获取时间基
    externalData.VideoTimeBase = formatContext->streams[videoIndex]->time_base;
    //FPS
    externalData.CurrentFps = av_q2d(formatContext->streams[videoIndex]->r_frame_rate);
    if (externalData.CurrentFps < InitParams.LimitFps) 
      externalData.CurrentFps = InitParams.LimitFps;
  }
  if (audioIndex != -1) {
    externalData.AudioTimeBase = formatContext->streams[audioIndex]->time_base;
  }


  //推流模式
  //***********************************

  if (pushMode) {
    LOGD("InitDecoder: push mode");

    //创建输出上下文
    ret = avformat_alloc_output_context2(&outputContext, NULL, pushType.c_str(), pushAddress.c_str());
    if (ret < 0) {
      SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avformat_alloc_output_context2 failed : %s", GetAvError(ret).c_str()).c_str());
      goto INIT_FAIL_CLEAN;
    }

    //配置输出流
    for (size_t i = 0; i < formatContext->nb_streams; i++) {
      //创建一个新的流
      auto outCodec = avcodec_find_decoder(formatContext->streams[i]->codecpar->codec_id);
      auto outStream = avformat_new_stream(outputContext, outCodec);
      if (!outStream) {
        SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avformat_new_stream failed : %s", GetAvError(ret).c_str()).c_str());
        goto INIT_FAIL_CLEAN;
      }
      //复制配置信息
      ret = avcodec_parameters_copy(outStream->codecpar, formatContext->streams[i]->codecpar);
      if (ret < 0) {
        SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_parameters_copy failed : %s", GetAvError(ret).c_str()).c_str());
        goto INIT_FAIL_CLEAN;
      }

      outStream->codecpar->codec_tag = 0;
    }

    //打开io
    ret = avio_open(&outputContext->pb, pushAddress.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
      SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avio_open failed : %s", GetAvError(ret).c_str()).c_str());
      goto INIT_FAIL_CLEAN;
    }
    //写文件头（Write file header）
    ret = avformat_write_header(outputContext, NULL);
    if (ret < 0) {
      SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avformat_write_header failed : %s", GetAvError(ret).c_str()).c_str());
      goto INIT_FAIL_CLEAN;
    }

    LOGD("InitDecoder: push mode init done");

    pushFrameIndex = 0;
    decodeState = CCDecodeState::Ready;
    return true;
  }

  //播放模式
  //***********************************

  else {
    videoCodecContext = nullptr;
    audioCodecContext = nullptr;

    //初始化视频解码器
    //***********************************
    if (videoIndex != -1) {

      codecParameters = formatContext->streams[videoIndex]->codecpar;
      videoCodec = avcodec_find_decoder(codecParameters->codec_id);

      if (videoCodec == nullptr) {
        LOGE("InitDecoder: Not find video decoder");
        SetLastError(VIDEO_PLAYER_ERROR_VIDEO_NOT_SUPPORT, "Not find video decoder");
        goto INIT_FAIL_CLEAN;
      }

      //通过解码器分配(并用  默认值   初始化)一个解码器context
      videoCodecContext = avcodec_alloc_context3(videoCodec);
      if (videoCodecContext == nullptr) {
        LOGE("InitDecoder: avcodec_alloc_context3 for videoCodecContext failed");
        SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, "avcodec_alloc_context3 for videoCodecContext failed");
        goto INIT_FAIL_CLEAN;
      }

      //更具指定的编码器值填充编码器上下文
      ret = avcodec_parameters_to_context(videoCodecContext, codecParameters);
      if (ret < 0) {
        LOGEF("InitDecoder: avcodec_parameters_to_context videoCodecContext failed : %s", GetAvError(ret).c_str());
        SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_parameters_to_context videoCodecContext failed : %s", GetAvError(ret).c_str()).c_str());
        goto INIT_FAIL_CLEAN;
      }

      hw_can_use = false;

      //初始化硬件解码
      if (type != AVHWDeviceType::AV_HWDEVICE_TYPE_NONE) {

        //遍历所有编解码器支持的硬件解码配置
        for (int i = 0;; i++) {
          const AVCodecHWConfig* config = avcodec_get_hw_config(videoCodec, i);
          if (!config) {
            SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR,
              StringHelper::FormatString(
                "InitDecoder: Decoder % s does not support device type % s.",
                videoCodec->name,
                av_hwdevice_get_type_name(type)
              ).c_str()
            );
            videoCodecContext->get_format = nullptr;
            type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;
            hw_can_use = false;
            goto SKIP_INIT_HW;
          }
          if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
            //把硬件支持的像素格式设置进去
            hw_pix_fmt = config->pix_fmt;
            break;
          }
        }

        //告诉解码器codec自己的目标像素格式是什么
        videoCodecContext->opaque = this;
        videoCodecContext->get_format = GetHwFormat;

        //初始化硬件解码
        if (InitHwDecoder(videoCodecContext, type) < 0) {
          LOGW("InitDecoder: InitHwDecoder failed, hardware decoder disabled");
          type = AVHWDeviceType::AV_HWDEVICE_TYPE_NONE;
          videoCodecContext->get_format = nullptr;
          hw_can_use = false;
          PostPlayerEventCallback(PLAYER_EVENT_INIT_HW_DECODER_FAIL);
        }
        else {
          hw_can_use = true;
        }
      }

    SKIP_INIT_HW:

      ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
      //通过所给的编解码器初始化编解码器上下文
      if (ret < 0) {
        LOGEF("InitDecoder: avcodec_open2 videoCodecContext failed : %s", GetAvError(ret).c_str());
        SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_open2 videoCodecContext failed : %s", GetAvError(ret).c_str()).c_str());
        goto INIT_FAIL_CLEAN;
      }
    }

    //初始化音频解码器
    //***********************************

    if (audioIndex > -1) {

      codecParameters = formatContext->streams[audioIndex]->codecpar;
      audioCodec = avcodec_find_decoder(codecParameters->codec_id);
      if (audioCodec == nullptr) {
        LOGW("InitDecoder: Not find audio decoder");
        goto AUDIO_INIT_DONE;
      }
      //通过解码器分配(并用  默认值   初始化)一个解码器context
      audioCodecContext = avcodec_alloc_context3(audioCodec);
      if (audioCodecContext == nullptr) {
        LOGW("avcodec_alloc_context3 for audioCodecContext failed");
        goto AUDIO_INIT_DONE;
      }

      //更具指定的编码器值填充编码器上下文
      ret = avcodec_parameters_to_context(audioCodecContext, codecParameters);
      if (ret < 0) {
        LOGWF("InitDecoder: avcodec_parameters_to_context audioCodecContext failed : %s", GetAvError(ret).c_str());
        goto AUDIO_INIT_DONE;
      }
      ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
      //通过所给的编解码器初始化编解码器上下文
      if (ret < 0) {
        LOGWF("InitDecoder: avcodec_open2 audioCodecContext failed : %s", GetAvError(ret).c_str());
        goto AUDIO_INIT_DONE;
      }

    }

  AUDIO_INIT_DONE:
    LOGDF("InitDecoder: audioCodecContext->codec_id : %d", audioCodecContext ? audioCodecContext->codec_id : 0);
    LOGDF("InitDecoder: videoCodecContext->codec_id : %d", videoCodecContext ? videoCodecContext->codec_id : 0);

    externalData.StartTime = formatContext->start_time * 1000 / AV_TIME_BASE;

    externalData.AudioCodecContext = audioCodecContext;
    externalData.VideoCodecContext = videoCodecContext;
    externalData.FormatContext = formatContext;

    decodeState = CCDecodeState::Ready;
    return true;
  }

INIT_FAIL_CLEAN:
  if (formatContext)
    avformat_close_input(&formatContext);
  if (outputContext)
    avformat_close_input(&outputContext);
  if (videoCodecContext)
    avcodec_free_context(&videoCodecContext);
  if (audioCodecContext)
    avcodec_free_context(&audioCodecContext);
  if (hw_device_ctx)
    av_buffer_unref(&hw_device_ctx);

  decodeState = CCDecodeState::FinishedWithError;
  return false;
}
bool CCVideoPlayer::DestroyDecoder() {

  LOGD("DestroyDecoder: Start");

  if (decodeState < CCDecodeState::Ready) {
    SetLastError(VIDEO_PLAYER_ERROR_NOR_INIT, "Decoder not init");
    LOGE("DestroyDecoder: Decoder not init");
    return false;
  }
  decodeState = CCDecodeState::NotInit;

  //停止线程
  StopDecoderThread();

  //释放资源
  if (videoCodecContext)
    avcodec_free_context(&videoCodecContext);
  if (audioCodecContext)
    avcodec_free_context(&audioCodecContext);
  if (formatContext)
    avformat_close_input(&formatContext);
  if (hw_device_ctx)
    av_buffer_unref(&hw_device_ctx);

  LOGD("DestroyDecoder: Done");
  return true;
}

//解码器线程控制
//**************************

void CCVideoPlayer::StartDecoderThread() {
  LOGDF("StartDecoderThread");

  decodeState = CCDecodeState::Decoding;

  if (decoderWorkerThread == nullptr) {
    decoderWorkerThread = new std::thread(DecoderWorkerThreadStub, this);
    decoderWorkerThread->detach();
  }

  if (!pushMode) {
    if (decoderVideoThread == nullptr) {
      decoderVideoThread = new std::thread(DecoderVideoThreadStub, this);
      decoderVideoThread->detach();
    }
    if (decoderAudioThread == nullptr) {
      decoderAudioThread = new std::thread(DecoderAudioThreadStub, this);
      decoderAudioThread->detach();
    }
  }
}
void CCVideoPlayer::StopDecoderThread() {
  LOGD("StopDecoderThread: Start");

  decodeState = CCDecodeState::Ready;
  if (decoderWorkerThread) {
    delete decoderWorkerThread;
    decoderWorkerThread = nullptr;
    decoderWorkerThreadDone.Wait();
  }

  if (!pushMode) {
    if (decoderVideoThread) {
      delete decoderVideoThread;
      decoderVideoThread = nullptr;
      decoderVideoThreadDone.Wait();
    }
    if (decoderAudioThread) {
      delete decoderAudioThread;
      decoderAudioThread = nullptr;
      decoderAudioThreadDone.Wait();
    }
  }

  LOGD("StopDecoderThread: Done");
}

//线程入口包装函数

void* CCVideoPlayer::DecoderWorkerThreadStub(void* param) {
  void* result = ((CCVideoPlayer*)param)->DecoderWorkerThread();
  ((CCVideoPlayer*)param)->decoderWorkerThread = 0;
  return result;
}
void* CCVideoPlayer::DecoderVideoThreadStub(void* param) {
  void* result = ((CCVideoPlayer*)param)->DecoderVideoThread();
  ((CCVideoPlayer*)param)->decoderVideoThread = 0;
  return result;
}
void* CCVideoPlayer::DecoderAudioThreadStub(void* param) {
  void* result = ((CCVideoPlayer*)param)->DecoderAudioThread();
  ((CCVideoPlayer*)param)->decoderAudioThread = 0;
  return result;
}

//线程主函数

//异步工作线程

void CCVideoPlayer::StartWorkerThread() {
  workerThreadEnable = true;
  if (workerThread)
    return;
  eventWorkerThreadQuit.Reset();
  workerThread = new std::thread(WorkerThread, this);
  workerThread->detach();
}
void CCVideoPlayer::StopWorkerThread()
{
  workerThreadEnable = false;
  if (workerThread) {
    delete workerThread;
    workerThread = nullptr;
    eventWorkerThreadQuit.Wait();
  }
}

int CCVideoPlayer::PostWorkerThreadCommand(int command, void* data) {

  auto task = new CCVideoPlayerAsyncTask();
  task->Command = command;
  if (command == VIDEO_PLAYER_ASYNC_TASK_OPEN)
    task->Path = (const char*)data;
  if (command == VIDEO_PLAYER_ASYNC_TASK_SET_STATE)
    task->State = (CCVideoState)(int)data;
  if (command == VIDEO_PLAYER_ASYNC_TASK_SET_POS)
    task->Pos = (int)data;
  
  workerQueue.Push(task);
  return task->Id;
}
void CCVideoPlayer::PostWorkerCommandFinish(CCAsyncTask* task) {
  CallPlayerEventCallback(PLAYER_EVENT_ASYNC_TASK, task);
}

void CCVideoPlayer::WorkerThread(CCVideoPlayer* self) {

  while (self->workerThreadEnable) {

    auto task = (CCVideoPlayerAsyncTask*)(self->workerQueue.Pop());

    if (task) {
      switch (task->Command)
      {
      case VIDEO_PLAYER_ASYNC_TASK_OPEN:
        task->ReturnStatus = self->OpenVideo(task->Path.c_str());
        break;
      case VIDEO_PLAYER_ASYNC_TASK_CLOSE:
        task->ReturnStatus = self->CloseVideo();
        break;
      case VIDEO_PLAYER_ASYNC_TASK_SET_STATE:
        task->ReturnStatus = self->SetVideoState(task->State);
        break;
      case VIDEO_PLAYER_ASYNC_TASK_GET_STATE:
        task->ReturnStatus = true;
        task->ReturnData = (void*)self->GetVideoState();
        break;
      case VIDEO_PLAYER_ASYNC_TASK_SET_POS:
        task->ReturnStatus = self->SetVideoPos(task->Pos);
        break;
      default:
        break;
      }

      if (!task->ReturnStatus) {
        task->ReturnErrorCode = self->GetLastError();
        task->ReturnErrorMessage = self->GetLastErrorMessageUtf8();
      }

      self->PostWorkerCommandFinish(task);

      if (!task->UserFree)
        delete task;
    }
    else if (
      self->decoderVideoFinish && self->decoderAudioFinish && 
      (self->render->NoMoreVideoFrame() || self->videoIndex == -1) &&
      self->videoState == CCVideoState::Playing
     ) {
      auto pos = self->GetVideoPos();
      auto dur = self->GetVideoLength();
      if (pos >= dur - 1000 || pos == -1) {

        self->decodeQueue.ClearAll();//清空数据
        self->StopAll();

        self->decodeState = CCDecodeState::Finished;
        self->DoSetVideoState(CCVideoState::Ended);

        self->PostPlayerEventCallback(PLAYER_EVENT_PLAY_END);
        LOGIF("PlayerWorkerThread: decodeState -> Finished pos/dur: %d/%d", pos, dur);
      }
    }

    auto postEventTask = (CCVideoPlayerPostEventAsyncTask*)(self->postBackQueue.Pop());
    if (postEventTask) {
      if (self->videoPlayerEventCallback)
        self->videoPlayerEventCallback(
          self, postEventTask->event, postEventTask->eventDataData,
          self->videoPlayerEventCallbackData
        );
      delete postEventTask;
    }

    Sleep(50);
  }

EXIT:
  self->workerQueue.Clear();
  self->eventWorkerThreadQuit.NotifyOne();
}

//主解码线程

void* CCVideoPlayer::DecoderWorkerThread() {
  //读取线程，解复用线程
  int ret;
  int start = 100;
  bool seeked = false;
  long long startTime = av_gettime();

  LOGIF("DecoderWorkerThread : Start: [%s]", CCDecodeStateToString(decodeState));

  decoderWorkerThreadDone.Reset();
  if (!decoderWorkerThreadLock.try_lock()) {
    LOGI("DecoderWorkerThread : Start twice");
    return nullptr;
  }


  while (decodeState == CCDecodeState::Decoding) {

    bool queueFull = false;
    auto maxMaxRenderQueueSize = InitParams.MaxRenderQueueSize;

    if (videoIndex == -1)
      queueFull = decodeQueue.AudioQueueSize() >= maxMaxRenderQueueSize;
    else 
      queueFull = decodeQueue.VideoQueueSize() >= maxMaxRenderQueueSize;

    if (queueFull) {
      av_usleep(10000);
      continue;
    }
    else if (start <= 0) {
      av_usleep(100); //目的是在刚开始时不延时，全速解码保证可以最快显示
    }
    else if (start > 0) {
      start--;
    }
    else {
      av_usleep(1000);
    }


    AVPacket* avPacket = decodeQueue.RequestPacket();
    if (!avPacket)
      break;
    ret = av_read_frame(formatContext, avPacket);
    //等于0成功，其它失败
    if (ret == 0) {

      if (pushMode) {
        //推流模式

        //没有封装格式的裸流（例如H.264裸流）计算并写入AVPacket的PTS，DTS，duration等参数
        if (avPacket->pts == AV_NOPTS_VALUE) {
          //Write PTS
          AVRational time_base1 = formatContext->streams[videoIndex]->time_base;
          //Duration between 2 frames (us)
          int64_t calc_duration = (int64_t)((double)AV_TIME_BASE / av_q2d(formatContext->streams[videoIndex]->r_frame_rate));
          //Parameters
          avPacket->pts = (int64_t)((double)(pushFrameIndex * calc_duration) / (double)(av_q2d(time_base1) * AV_TIME_BASE));
          avPacket->dts = avPacket->pts;
          avPacket->duration = (int64_t)((double)calc_duration / (double)(av_q2d(time_base1) * AV_TIME_BASE));
        }

        //计算转换时间戳 获取时间基数
        AVRational itime = formatContext->streams[avPacket->stream_index]->time_base;
        AVRational otime = outputContext->streams[avPacket->stream_index]->time_base;
        avPacket->pts = av_rescale_q_rnd(avPacket->pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        avPacket->dts = av_rescale_q_rnd(avPacket->dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        //到这一帧经历了多长时间
        avPacket->duration = av_rescale_q_rnd(avPacket->duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        avPacket->pos = -1;

        //视频帧推送速度
        if (avPacket->stream_index == videoIndex)
        {
          AVRational tb = formatContext->streams[avPacket->stream_index]->time_base;
          //已经过去的时间
          long long now = av_gettime() - startTime;
          long long dts = 0;
          dts = avPacket->dts * (int64_t)(1000 * 1000 * av_q2d(tb));
          if (dts > now)
            av_usleep((uint32_t)(dts - now));
          pushFrameIndex++;
        }

        ret = av_interleaved_write_frame(outputContext, avPacket);

        decodeQueue.ReleasePacket(avPacket);

        if (ret < 0) {
          LOGEF("DecoderWorkerThread (Push) : av_interleaved_write_frame failed : %s", GetAvError(ret).c_str());
          decodeQueue.ReleasePacket(avPacket);
          decodeState = CCDecodeState::FinishedWithError;
          break;
        }
      }
      else {
        //正常模式
        if (avPacket->stream_index == audioIndex)
          decodeQueue.AudioEnqueue(avPacket);//添加音频包至队列中
        else if (avPacket->stream_index == videoIndex)
          decodeQueue.VideoEnqueue(avPacket);//添加视频包至队列中
        else
          decodeQueue.ReleasePacket(avPacket);
      }
      seeked = false;
    }
    else if (ret == AVERROR_EOF) {
      decodeQueue.ReleasePacket(avPacket);

      //读取完成，但是可能还没有播放完成，等待播放完成后再执行操作
      if (
        pushMode || (
        decodeQueue.VideoFrameQueueSize() == 0 && 
        decodeQueue.AudioFrameQueueSize() == 0)
      ) {

        //如果循环，则跳到第一帧播放
        if (loop) {
          if (!seeked) {
            ret = av_seek_frame(formatContext, -1, formatContext->start_time, 0);
            if (ret < 0)
              LOGEF("DecoderWorkerThread : av_seek_frame failed : %s", GetAvError(ret).c_str());
            seeked = true;
          }
        }
        else {
          //不循环，结束状态
          decodeState = CCDecodeState::Finish;
          break;
        }
      }

      av_usleep(100);
    }
    else {
      LOGEF("DecoderWorkerThread : av_read_frame failed : %s", GetAvError(ret).c_str());
      decodeQueue.ReleasePacket(avPacket);
      decodeState = CCDecodeState::FinishedWithError;
      break;
    }
  }

  LOGIF("DecoderWorkerThread : End [%s]", CCDecodeStateToString(decodeState));

  decoderWorkerThreadLock.unlock();
  decoderWorkerThreadDone.NotifyOne();
  return nullptr;
}
void* CCVideoPlayer::DecoderVideoThread() {
  //视频解码线程
  LOGIF("DecoderVideoThread : Start: [%s]", CCDecodeStateToString(decodeState));

  decoderVideoThreadDone.Reset();
  if (!decoderVideoThreadLock.try_lock()) {
    LOGI("DecoderVideoThread : Start twice");
    return nullptr;
  }

  int ret;
  AVPacket* packet;
  AVFrame* frame;
  auto maxMaxRenderQueueSize = InitParams.MaxRenderQueueSize;
  while (decodeState >= CCDecodeState::Decoding) {

    //延时
    if (decodeQueue.VideoFrameQueueSize() > maxMaxRenderQueueSize) {
      av_usleep(1000 * 100);
      continue;
    }

    packet = decodeQueue.VideoDequeue();
    if (!packet) {

      //如果主线程标记已经结束，那么没有收到包即意味着结束，退出线程
      if (!decoderVideoFinish && decodeState == CCDecodeState::Finish) {
        decoderVideoFinish = true;
        LOGI("DecoderVideoThread : Finish");
        goto QUIT;
      }
      if (decoderVideoFinish) goto QUIT;

      av_usleep(1000 * 50);
      continue;
    }

    //把包丢给解码器
    ret = avcodec_send_packet(videoCodecContext, packet);

    if (ret == AVERROR(EAGAIN)) {
      decodeQueue.VideoQueueBack(packet);
      goto RECEIVE;
    }
    else if (ret != 0) {
      decodeQueue.ReleasePacket(packet);
      LOGEF("DecoderVideoThread : avcodec_send_packet failed : %s", GetAvError(ret).c_str());
      break;
    }

    decodeQueue.ReleasePacket(packet);

  RECEIVE:
    frame = decodeQueue.RequestFrame();
    if (!frame)
      break;
    do {
      ret = avcodec_receive_frame(videoCodecContext, frame);
      if (ret >= 0) {

        //硬件加速帧的数据处理
        if (frame->format == hw_pix_fmt) {
          auto sw_frame = decodeQueue.RequestFrame();
          //把数据从GPU传输到CPU
          if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
            //失败
            decodeQueue.ReleaseFrame(sw_frame);
            LOGEF("DecoderVideoThread : av_hwframe_transfer_data failed : %s", GetAvError(ret).c_str());
            continue;
          }
          else {
            av_frame_copy_props(sw_frame, frame);
            hw_frame_pix_fmt = (AVPixelFormat)sw_frame->format;
            decodeQueue.VideoFrameEnqueue(sw_frame);
            decodeQueue.ReleaseFrame(frame);
          }
        }
        else {
          hw_frame_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
          //入队列
          decodeQueue.VideoFrameEnqueue(frame);
        }

        frame = decodeQueue.RequestFrame();
      }
      else {
        decodeQueue.ReleaseFrame(frame);
        if (ret != AVERROR(EAGAIN))
          LOGEF("DecoderVideoThread : avcodec_receive_frame failed : %s", GetAvError(ret).c_str());
      }
    } while (ret == 0);

  }

QUIT:
  LOGIF("DecoderVideoThread : End [%s]", CCDecodeStateToString(decodeState));

  decoderVideoThreadLock.unlock();
  decoderVideoThreadDone.NotifyOne();
  return nullptr;
}
void* CCVideoPlayer::DecoderAudioThread() {
  //音频解码线程
  LOGIF("DecoderAudioThread : Start: [%s]", CCDecodeStateToString(decodeState));

  decoderAudioThreadDone.Reset();
  if (!decoderAudioThreadLock.try_lock()) {
    LOGI("DecoderAudioThread : Start twice");
    return nullptr;
  }

  int ret;
  AVPacket* packet;
  AVFrame* frame;
  auto maxMaxRenderQueueSize = InitParams.MaxRenderQueueSize;
  while (decodeState >= CCDecodeState::Decoding) {

    if (decodeQueue.AudioFrameQueueSize() > maxMaxRenderQueueSize) {
      av_usleep(1000 * 10);
      continue;
    }

    packet = decodeQueue.AudioDequeue();
    //延时
    if (!packet) {

      //如果主线程标记已经结束，那么没有收到包即意味着结束，退出线程
      if (!decoderAudioFinish && decodeState == CCDecodeState::Finish) {
        decoderAudioFinish = true;
        LOGI("DecoderAudioThread : Finish");
        goto QUIT;
      }
      if (decoderAudioFinish)
        goto QUIT;

      av_usleep(1000 * 10);
      continue;
    }

    //把包丢给解码器
    ret = avcodec_send_packet(audioCodecContext, packet);

    if (ret == AVERROR(EAGAIN)) {
      decodeQueue.AudioQueueBack(packet);
      goto RECEIVE;
    }
    else if (ret != 0) {
      LOGEF("DecoderAudioThread : avcodec_send_packet failed : %s", GetAvError(ret).c_str());
      break;
    }

    decodeQueue.ReleasePacket(packet);

  RECEIVE:
    frame = decodeQueue.RequestFrame();
    if (!frame)
      break;
    do {
      ret = avcodec_receive_frame(audioCodecContext, frame);
      if (ret == 0) {
        //入队列
        decodeQueue.AudioFrameEnqueue(frame);
        frame = decodeQueue.RequestFrame();
      }
      else {
        decodeQueue.ReleaseFrame(frame);
        if (ret != AVERROR(EAGAIN))
          LOGEF("DecoderAudioThread : avcodec_receive_frame failed : %s", GetAvError(ret).c_str());
      }
    } while (ret == 0);
  }

QUIT:
  LOGIF("DecoderAudioThread : End [%s]", CCDecodeStateToString(decodeState));

  decoderAudioThreadLock.unlock();
  decoderAudioThreadDone.NotifyOne();
  return nullptr;
}

//同步渲染

CCVideoPlayerCallbackDeviceData* CCVideoPlayer::SyncRenderStart()
{
  if (render->GetState() == CCRenderState::NotRender)
    return nullptr;
  return render->SyncRenderStart();
}
void CCVideoPlayer::SyncRenderEnd() {
  if (render->GetState() == CCRenderState::NotRender)
    return;
  render->SyncRenderEnd();
}

//更新画面缓冲区大小
void CCVideoPlayer::RenderUpdateDestSize(int width, int height) {
  externalData.InitParams->DestWidth = width;
  externalData.InitParams->DestHeight = height;
  render->UpdateDestSize();
}

AVPixelFormat CCVideoPlayer::GetVideoPixelFormat()
{
  return hw_frame_pix_fmt != AVPixelFormat::AV_PIX_FMT_NONE ? 
    hw_frame_pix_fmt : (
      (videoCodecContext ? videoCodecContext->pix_fmt : AVPixelFormat::AV_PIX_FMT_NONE)
    );
}

void CCVideoPlayer::Init(CCVideoPlayerInitParams* initParams) {
  if (initParams != nullptr)
    memcpy(&InitParams, initParams, sizeof(CCVideoPlayerInitParams));
  render = new CCPlayerRender();
  externalData.InitParams = &InitParams;
  externalData.DecodeQueue = &decodeQueue;
  externalData.Player = this;

  decodeQueue.Init(&externalData);

  StartWorkerThread();

  //av_log

  av_log_set_level(AV_LOG_WARNING);
  av_log_set_callback(FFmpegLogFunc);
}
void CCVideoPlayer::Destroy() {
  StopWorkerThread();

  decodeQueue.Destroy();

  if (render) {
    render->Destroy();
    delete render;
    render = nullptr;
  }

}
void CCVideoPlayer::GlobalInit() {

}

void CCVideoPlayer::FFmpegLogFunc(void* ptr, int level, const char* fmt, va_list vl) {
  UNREFERENCED_PARAMETER(ptr);
  if (level <= AV_LOG_WARNING)
    vprintf_s(fmt, vl);
}

void CCVideoPlayer::SetLastError(int code, const wchar_t* errmsg)
{
  LOGEF("CCVideoPlayer Error: %s (%d)", StringHelper::UnicodeToAnsi(errmsg).c_str(), code);
  lastErrorMessage = errmsg ? errmsg : L"";
  lastErrorCode = code;
}
void CCVideoPlayer::SetLastError(int code, const char* errmsg)
{
  LOGEF("CCVideoPlayer Error: %s (%d)", errmsg, code);
  lastErrorMessage = errmsg ? StringHelper::AnsiToUnicode(errmsg) : L"";
  lastErrorCode = code;
}
const wchar_t* CCVideoPlayer::GetLastErrorMessage()
{
  return lastErrorMessage.c_str();
}
const char* CCVideoPlayer::GetLastErrorMessageUtf8()
{
  lastErrorMessageUtf8 = StringHelper::UnicodeToUtf8(lastErrorMessage);
  return lastErrorMessageUtf8.c_str();
}










