//
// Created by roger on 2020/12/19.
//

#include "pch.h"
#include "CCVideoPlayer.h"
#include "CCPlayerDefine.h"
#include "StringHelper.h"
#include "Logger.h"

void CCVideoPlayer::CallPlayerEventCallback(int message) {
  CallPlayerEventCallback(message, nullptr);
}
void CCVideoPlayer::CallPlayerEventCallback(int message, void* data) {
  if (videoPlayerEventCallback != nullptr)
    videoPlayerEventCallback(this, message, data, videoPlayerEventCallbackData);
}
int CCVideoPlayer::GetLastError() const { return lastErrorCode; }

//播放器子线程方法
//**************************

void CCVideoPlayer::DoOpenVideo() {
  LOGD("DoOpenVideo: Start");

  DoSetVideoState(CCVideoState::Loading);

  if (!InitDecoder()) {
    DoSetVideoState(CCVideoState::Failed);
    CallPlayerEventCallback(PLAYER_EVENT_OPEN_FAIED);
    LOGD("DoOpenVideo: InitDecoder failed");
    return;
  }

  decodeQueue.Init(&externalData);

  CallPlayerEventCallback(PLAYER_EVENT_INIT_DECODER_DONE);
  LOGD("DoOpenVideo: InitDecoder done");

  if (!render->Init(&externalData)) {
    DoSetVideoState(CCVideoState::Failed);
    CallPlayerEventCallback(PLAYER_EVENT_OPEN_FAIED);
    LOGD("DoOpenVideo: Init render failed");
    return;
  }

  DoSetVideoState(CCVideoState::Opened);
  LOGD("DoOpenVideo: Done");
  openDoneEvent.NotifyOne();
  CallPlayerEventCallback(PLAYER_EVENT_OPEN_DONE);
}
void CCVideoPlayer::DoCloseVideo() {
  LOGD("DoCloseVideo: Start");

  if (videoState != CCVideoState::NotOpen) {

    //停止
    StopAll();

    //释放
    DestroyDecoder();
    render->Destroy();
    decodeQueue.Destroy();

    DoSetVideoState(CCVideoState::NotOpen);
    closeDoneEvent.NotifyOne();
  }

  CallPlayerEventCallback(PLAYER_EVENT_CLOSED);
  LOGD("DoCloseVideo: Done");
}
void CCVideoPlayer::DoSeekVideo() {

  if (videoState == CCVideoState::Playing) {
    LOGD("Seek: Stop before seek");

    //先停止序列
    StopDecoderThread();
    render->Stop();
  }

  LOGDF("Seek: Seek to %d", seekDest);

  //跳转到指定帧

  int ret;
  {

    seekPosVideo = av_rescale_q(seekDest * 1000, av_get_time_base_q(), formatContext->streams[videoIndex]->time_base);
    if (seekPosVideo > render->GetCurVideoPts())
      ret = av_seek_frame(formatContext, videoIndex, seekPosVideo, AVSEEK_FLAG_FRAME);
    else
      ret = av_seek_frame(formatContext, videoIndex, seekPosVideo, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    if (ret != 0) {
      LOGEF("av_seek_frame video failed : %d", ret);
      playerSeeking = 0;
      return;
    }

    //avcodec_flush_buffers(videoCodecContext);//清空缓存数据
  }

  /*
  if(audioIndex != -1) {
      seekPosAudio = av_rescale_q(seekDest * 1000, AV_TIME_BASE_Q, formatContext->streams[audioIndex]->time_base);
      ret = av_seek_frame(formatContext, audioIndex, seekPosAudio,
                              (uint) AVSEEK_FLAG_BACKWARD | (uint) AVSEEK_FLAG_ANY);
      if (ret != 0)
          LOGEF("av_seek_frame audio failed : %d", ret);
      else
          avcodec_flush_buffers(audioCodecContext);//清空缓存数据
  }
  */

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

  playerSeeking = 0;
  CallPlayerEventCallback(PLAYER_EVENT_SEEK_DONE);
  LOGD("Seek: Seek done");
}
void CCVideoPlayer::DoSetVideoState(CCVideoState state) {
  setVideoStateLock.lock();
  videoState = state;
  setVideoStateLock.unlock();
}

//播放器公共方法
//**************************

bool CCVideoPlayer::OpenVideo(const char* filePath) {

  LOGDF("OpenVideo: Call. %hs", StringHelper::Utf8ToUnicode(filePath).c_str());

  if (videoState == CCVideoState::Loading) {
    SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "Player is loading, please wait a second");
    return false;
  }
  if (videoState > CCVideoState::NotOpen) {
    LOGEF("A video has been opened. Please close it first [%s]", CCVideoStateToString(videoState));
    SetLastError(VIDEO_PLAYER_ERROR_ALREADY_OPEN, "A video has been opened. Please close it first");
    return false;
  }

  currentFile = filePath;
  DoSetVideoState(CCVideoState::Loading);

  openDoneEvent.Reset();

  if (playerOpen == 0) {
    playerOpen = 1;
    return true;
  }
  return true;
}
bool CCVideoPlayer::OpenVideo(const wchar_t* filePath) {
  return OpenVideo(StringHelper::UnicodeToUtf8(filePath).c_str());
}
bool CCVideoPlayer::CloseVideo() {
  LOGD("CloseVideo: Call");

  if (videoState == CCVideoState::NotOpen || videoState == CCVideoState::Failed) {
    LOGE("Can not close video because video not load");
    return false;
  }

  closeDoneEvent.Reset();

  if (playerClose == 0) {
    playerClose = 1;
    return true;
  }
  return true;
}
void CCVideoPlayer::WaitOpenVideo() {
  if (playerOpen)
    openDoneEvent.Wait();
}
void CCVideoPlayer::WaitCloseVideo() {
  if (playerClose)
    closeDoneEvent.Wait();
}
void CCVideoPlayer::SetVideoState(CCVideoState newState) {
  if (setVideoStateLock.try_lock())
    setVideoStateLock.unlock();
  if (videoState == newState)
    return;

  LOGDF("SetVideoState : %s", CCVideoStateToString(newState));

  switch (newState) {
  case CCVideoState::NotOpen: CloseVideo(); break;
  case CCVideoState::Playing:
    if (videoState == CCVideoState::Loading) {
      LOGEF("Now player is loading");
      SetLastError(VIDEO_PLAYER_ERROR_NOW_IS_LOADING, "Now player is loading");
      return;
    }
    if (GetVideoPos() > GetVideoLength() - 500)
      SetVideoPos(0);
    StartAll();
    break;
  case CCVideoState::Paused: StopAll(); break;
  case CCVideoState::Ended:
  case CCVideoState::Failed:
  case CCVideoState::Loading:
    LOGEF("Bad state %s, this state can only get.", CCVideoStateToString(newState));
    SetLastError(VIDEO_PLAYER_ERROR_STATE_CAN_ONLY_GET, "");
    return;
  }

}
void CCVideoPlayer::SetVideoPos(int64_t pos) {
  if (playerSeeking == 0) {
    playerSeeking = 1;
    seekDest = externalData.StartTime + pos;
    LOGDF("SetVideoPos: Call. %ld/%ld", seekDest, GetVideoLength());
  }
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

  if (audioIndex != -1)
    return (int64_t)(render->GetCurAudioPts() * av_q2d(formatContext->streams[audioIndex]->time_base) * 1000);
  else
    return (int64_t)(render->GetCurVideoPts() * av_q2d(formatContext->streams[videoIndex]->time_base) * 1000);
}
bool CCVideoPlayer::GetVideoLoop() { return loop; }
CCVideoState CCVideoPlayer::GetVideoState() { 
  if (setVideoStateLock.try_lock())
    setVideoStateLock.unlock();
  return videoState;
}
int64_t CCVideoPlayer::GetVideoLength() {
  if (!formatContext) {
    SetLastError(VIDEO_PLAYER_ERROR_NOT_OPEN, "Video not open");
    LOGE("Video not open");
    return 0;
  }
  return (int64_t)(formatContext->duration / (double)AV_TIME_BASE * (double)1000); //ms
}
void CCVideoPlayer::SetVideoVolume(int vol) { render->SetVolume(vol); }
void CCVideoPlayer::SetVideoLoop(bool loop)
{
  this->loop = loop;
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

  startAllLock.lock();

  decoderAudioFinish = audioIndex == -1;
  decoderVideoFinish = false;
  StartDecoderThread();
  render->Start();

  startAllLock.unlock();
}
void CCVideoPlayer::StopAll() {
  if (videoState == CCVideoState::Paused || videoState == CCVideoState::Ended)
    return;
  DoSetVideoState(CCVideoState::Paused);

  stopAllLock.lock();

  StopDecoderThread();
  render->Stop();

  stopAllLock.unlock();
}

//解码器初始化与反初始化
//**************************

bool CCVideoPlayer::InitDecoder() {

  int ret;
  decodeState = CCDecodeState::Preparing;

  formatContext = avformat_alloc_context();
  //打开视频数据源
  int openState = avformat_open_input(&formatContext, currentFile.c_str(), nullptr, nullptr);
  if (openState < 0) {
    char errBuf[128];
    if (av_strerror(openState, errBuf, sizeof(errBuf)) == 0) {
      LOGEF("InitDecoder: Failed to open input file, error : %s", errBuf);
      SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("Failed to open input file, error : %s", errBuf).c_str());
    }
    return false;
  }
  //为分配的AVFormatContext 结构体中填充数据
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, "Failed to read the input video stream information");
    LOGE("InitDecoder: Failed to read the input video stream information");
    return false;
  }

  videoIndex = -1;
  audioIndex = -1;

  //LOGD("---------------- File Information ---------------");
  //av_dump_format(formatContext, 0, currentFile.c_str(), 0);
  //LOGD("-------------- File Information end -------------");

  //找到"视频流".AVFormatContext 结构体中的nb_streams字段存储的就是当前视频文件中所包含的总数据流数量——
  //视频流，音频流
  //***********************************

  for (uint32_t i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      LOGDF("InitDecoder: video stream : %d", i);
    } else if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      LOGDF("InitDecoder: audio stream : %d", i);
    }
  }

  for (uint32_t i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoIndex = i;
      break;
    }
  }
  for (uint32_t i = 0; i < formatContext->nb_streams; i++) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audioIndex = i;
      break;
    }
  }
  if (videoIndex == -1) {
    SetLastError(VIDEO_PLAYER_ERROR_NO_VIDEO_STREAM, "Not found video stream");
    LOGE("InitDecoder: Not found video stream!");
    return false;
  }

  LOGDF("InitDecoder: streams count : %d", formatContext->nb_streams);
  LOGDF("InitDecoder: audioIndex : %d", audioIndex);
  LOGDF("InitDecoder: videoIndex : %d", videoIndex);

  externalData.VideoTimeBase = formatContext->streams[videoIndex]->time_base;
  if (audioIndex != -1)
    externalData.AudioTimeBase = formatContext->streams[audioIndex]->time_base;

  //FPS
  externalData.CurrentFps = av_q2d(formatContext->streams[videoIndex]->r_frame_rate);
  if (externalData.CurrentFps < InitParams.LimitFps) externalData.CurrentFps = InitParams.LimitFps;

  //初始化视频解码器
  //***********************************

  AVCodecParameters* codecParameters = formatContext->streams[videoIndex]->codecpar;
  videoCodec = avcodec_find_decoder(codecParameters->codec_id);

  if (videoCodec == nullptr) {
    LOGE("InitDecoder: Not find video decoder");
    SetLastError(VIDEO_PLAYER_ERROR_VIDEO_NOT_SUPPORT, "Not find video decoder");
    return false;
  }

  //通过解码器分配(并用  默认值   初始化)一个解码器context
  videoCodecContext = avcodec_alloc_context3(videoCodec);
  if (videoCodecContext == nullptr) {
    LOGE("InitDecoder: avcodec_alloc_context3 for videoCodecContext failed");
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, "avcodec_alloc_context3 for videoCodecContext failed");
    return false;
  }

  //更具指定的编码器值填充编码器上下文
  ret = avcodec_parameters_to_context(videoCodecContext, codecParameters);
  if (ret < 0) {
    LOGEF("InitDecoder: avcodec_parameters_to_context videoCodecContext failed : %d", ret);
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_parameters_to_context videoCodecContext failed : %d", ret).c_str());
    return false;
  }
  ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
  //通过所给的编解码器初始化编解码器上下文
  if (ret < 0) {
    LOGEF("InitDecoder: avcodec_open2 videoCodecContext failed : %d", ret);
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_open2 videoCodecContext failed : %d", ret).c_str());
    return false;
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
      LOGWF("InitDecoder: avcodec_parameters_to_context audioCodecContext failed : %d", ret);
      goto AUDIO_INIT_DONE;
    }
    ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
    //通过所给的编解码器初始化编解码器上下文
    if (ret < 0) {
      LOGWF("InitDecoder: avcodec_open2 audioCodecContext failed : %d", ret);
      goto AUDIO_INIT_DONE;
    }

  }
  else audioCodecContext = nullptr;
AUDIO_INIT_DONE:

  LOGDF("InitDecoder: audioCodecContext->codec_id : %d", audioCodecContext ? audioCodecContext->codec_id : 0);
  LOGDF("InitDecoder: videoCodecContext->codec_id : %d", videoCodecContext->codec_id);

  externalData.StartTime = formatContext->start_time * 1000 / AV_TIME_BASE;

  externalData.AudioCodecContext = audioCodecContext;
  externalData.VideoCodecContext = videoCodecContext;
  externalData.FormatContext = formatContext;

  decodeState = CCDecodeState::Ready;
  return true;
}
bool CCVideoPlayer::DestroyDecoder() {

  LOGE("DestroyDecoder: Start");

  if (decodeState < CCDecodeState::Ready) {
    SetLastError(VIDEO_PLAYER_ERROR_NOR_INIT, "Decoder not init");
    LOGE("DestroyDecoder: Decoder not init");
    return false;
  }

  //停止线程
  StopDecoderThread();

  //释放资源
  avcodec_free_context(&videoCodecContext);
  if (audioIndex != -1)
    avcodec_free_context(&audioCodecContext);
  avformat_close_input(&formatContext);
  avformat_free_context(formatContext);

  LOGE("DestroyDecoder: Done");
  decodeState = CCDecodeState::NotInit;
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
  if (decoderVideoThread == nullptr) {
    decoderVideoThread = new std::thread(DecoderVideoThreadStub, this);
    decoderVideoThread->detach();
  }
  if (decoderAudioThread == nullptr) {
    decoderAudioThread = new std::thread(DecoderAudioThreadStub, this);
    decoderAudioThread->detach();
  }
}
void CCVideoPlayer::StopDecoderThread() {
  LOGD("StopDecoderThread: Start");

  decodeState = CCDecodeState::Ready;
  if (decoderWorkerThread) {
    delete decoderWorkerThread;
    decoderWorkerThread = nullptr;
    decoderWorkerThreadLock.lock();
    decoderWorkerThreadLock.unlock();
  }
  if (decoderVideoThread) {
    delete decoderVideoThread;
    decoderVideoThread = nullptr;
    decoderVideoThreadLock.lock();
    decoderVideoThreadLock.unlock();
  }
  if (decoderAudioThread) {
    delete decoderAudioThread;
    decoderAudioThread = nullptr;
    decoderAudioThreadLock.lock();
    decoderAudioThreadLock.unlock();
  }

  LOGD("StopDecoderThread: Done");
}

//线程入口包装函数

void* CCVideoPlayer::PlayerWorkerThreadStub(void* param) {
  void* result = ((CCVideoPlayer*)param)->PlayerWorkerThread();
  ((CCVideoPlayer*)param)->playerWorkerThread = 0;
  return result;
}
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

void* CCVideoPlayer::PlayerWorkerThread() {
  //背景线程，用于防止用户主线程卡顿
  LOGI("PlayerWorkerThread : Start");

  while (playerWorking) {

    if (playerClose == 1 && videoState != CCVideoState::Loading) {
      playerClose = 2;
      DoCloseVideo();
      playerClose = 0;
    }
    if (playerOpen == 1) {
      playerOpen = 2;
      DoOpenVideo();
      playerOpen = 0;
    }
    if (playerSeeking == 1) {
      playerSeeking = 2;
      DoSeekVideo();
    }
    if (playerSeeking != 2 && decoderVideoFinish && decoderAudioFinish &&
      (decodeState > CCDecodeState::NotInit && decodeState != CCDecodeState::Finished)) {
      auto pos = GetVideoPos();
      auto dur = GetVideoLength();
      if (pos >= dur - 30 || pos == -1) {

        decodeQueue.ClearAll();//清空数据
        StopAll();

        decodeState = CCDecodeState::Finished;
        DoSetVideoState(CCVideoState::Ended);

        CallPlayerEventCallback(PLAYER_EVENT_PLAY_DONE);
        LOGIF("PlayerWorkerThread: decodeState -> Finished pos: %d dur: %d", pos, dur);
      }
    }

    av_usleep(100 * 1000);
  }

  LOGI("PlayerWorkerThread : End");
  return nullptr;
}
void* CCVideoPlayer::DecoderWorkerThread() {
  //读取线程，解复用线程
  int ret;
  int start = 100;
  bool seeked = false;
  LOGIF("DecoderWorkerThread : Start: [%s]", CCDecodeStateToString(decodeState));

  if (!decoderWorkerThreadLock.try_lock()) {
    LOGI("DecoderWorkerThread : Start twice");
    return nullptr;
  }

  while (decodeState == CCDecodeState::Decoding) {

    auto maxMaxRenderQueueSize =  InitParams.MaxRenderQueueSize;
    if (
      decodeQueue.AudioQueueSize() > maxMaxRenderQueueSize || 
      decodeQueue.VideoQueueSize() > maxMaxRenderQueueSize
    ) {
      av_usleep(10000);
      continue;
    }
    else if (start <= 0) {
      av_usleep(100);
    }
    else {
      start--;
    }

    AVPacket* avPacket = decodeQueue.RequestPacket();
    if (!avPacket)
      break;
    ret = av_read_frame(formatContext, avPacket);
    //等于0成功，其它失败
    if (ret == 0) {
      if (avPacket->stream_index == audioIndex)
        decodeQueue.AudioEnqueue(avPacket);//添加音频包至队列中
      else if (avPacket->stream_index == videoIndex)
        decodeQueue.VideoEnqueue(avPacket);//添加视频包至队列中
      else
        decodeQueue.ReleasePacket(avPacket);
      seeked = false;
    }
    else if (ret == AVERROR_EOF) {
      decodeQueue.ReleasePacket(avPacket);

      //读取完成，但是可能还没有播放完成，等待播放完成后再执行操作
      if (
        decodeQueue.VideoFrameQueueSize() == 0 && 
        decodeQueue.AudioFrameQueueSize() == 0
      ) {

        //如果循环，则跳到第一帧播放
        if (loop) {
          if (!seeked) {
            ret = av_seek_frame(formatContext, -1, formatContext->start_time, 0);
            if (ret < 0)
              LOGEF("DecoderWorkerThread : av_seek_frame failed : %d", ret);
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
      LOGEF("DecoderWorkerThread : av_read_frame failed : %d", ret);
      decodeQueue.ReleasePacket(avPacket);
      decodeState = CCDecodeState::FinishedWithError;
      break;
    }
  }

  LOGIF("DecoderWorkerThread : End [%s]", CCDecodeStateToString(decodeState));

  decoderWorkerThreadLock.unlock();
  return nullptr;
}
void* CCVideoPlayer::DecoderVideoThread() {
  //视频解码线程
  LOGIF("DecoderVideoThread : Start: [%s]", CCDecodeStateToString(decodeState));

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
      LOGEF("DecoderVideoThread : avcodec_send_packet failed : %d", ret);
      break;
    }

    decodeQueue.ReleasePacket(packet);

  RECEIVE:
    frame = decodeQueue.RequestFrame();
    if (!frame)
      break;
    do {
      ret = avcodec_receive_frame(videoCodecContext, frame);
      if (ret == 0) {
        //入队列
        decodeQueue.VideoFrameEnqueue(frame);
        frame = decodeQueue.RequestFrame();
      }
      else {
        decodeQueue.ReleaseFrame(frame);
        if (ret != AVERROR(EAGAIN))
          LOGEF("DecoderVideoThread : avcodec_receive_frame failed : %d", ret);
      }
    } while (ret == 0);

  }

QUIT:
  LOGIF("DecoderVideoThread : End [%s]", CCDecodeStateToString(decodeState));

  decoderVideoThreadLock.unlock();
  return nullptr;
}
void* CCVideoPlayer::DecoderAudioThread() {
  //音频解码线程
  LOGIF("DecoderAudioThread : Start: [%s]", CCDecodeStateToString(decodeState));

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
      av_usleep(1000 * 20);
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
      LOGEF("DecoderAudioThread : avcodec_send_packet failed : %d", ret);
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
          LOGEF("DecoderAudioThread : avcodec_receive_frame failed : %d", ret);
      }
    } while (ret == 0);
  }

QUIT:
  LOGIF("DecoderAudioThread : End [%s]", CCDecodeStateToString(decodeState));

  decoderAudioThreadLock.unlock();
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
}

CCVideoPlayer::CCVideoPlayer() {
  Init(nullptr);
}
CCVideoPlayer::CCVideoPlayer(CCVideoPlayerInitParams* initParams) {
  Init(initParams);
}
CCVideoPlayer::~CCVideoPlayer() {
  Destroy();
}
void CCVideoPlayer::Init(CCVideoPlayerInitParams* initParams) {
  if (initParams != nullptr)
    memcpy(&InitParams, initParams, sizeof(CCVideoPlayerInitParams));
  render = new CCPlayerRender();
  externalData.InitParams = &InitParams;
  externalData.DecodeQueue = &decodeQueue;
  externalData.Player = this;

  playerWorking = true;

  if (playerWorkerThread == nullptr) {
    playerWorkerThread = new std::thread(PlayerWorkerThreadStub, this);
    playerWorkerThread->detach();
  }

  //av_log

  av_log_set_level(AV_LOG_WARNING);
  av_log_set_callback(FFmpegLogFunc);
}
void CCVideoPlayer::Destroy() {
  playerWorking = false;

  if (playerWorkerThread) {
    delete playerWorkerThread;
    playerWorkerThread = nullptr;
  }
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
  LOGEF("CCVideoPlayer Error: %d, %s", code, StringHelper::UnicodeToAnsi(errmsg).c_str());
  lastErrorMessage = errmsg ? errmsg : L"";
  lastErrorCode = code;
}
void CCVideoPlayer::SetLastError(int code, const char* errmsg)
{
  LOGEF("CCVideoPlayer Error: %d, %s", code, errmsg);
  lastErrorMessage = errmsg ? StringHelper::AnsiToUnicode(errmsg) : L"";
  lastErrorCode = code;
}
const wchar_t* CCVideoPlayer::GetLastErrorMessage()
{
  return lastErrorMessage.c_str();
}










