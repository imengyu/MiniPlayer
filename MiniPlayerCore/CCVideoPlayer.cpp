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
  LOGD("DoOpenVideo");

  videoState = CCVideoState::Loading;

  if (!InitDecoder()) {
    videoState = CCVideoState::Failed;
    CallPlayerEventCallback(PLAYER_EVENT_OPEN_FAIED);
    return;
  }

  decodeQueue.Init(&externalData);

  CallPlayerEventCallback(PLAYER_EVENT_INIT_DECODER_DONE);

  if (!render->Init(&externalData)) {
    videoState = CCVideoState::Failed;
    CallPlayerEventCallback(PLAYER_EVENT_OPEN_FAIED);
    return;
  }

  videoState = CCVideoState::Opened;
  CallPlayerEventCallback(PLAYER_EVENT_OPEN_DONE);
}
void CCVideoPlayer::DoCloseVideo() {
  LOGD("DoCloseVideo");

  //停止
  StopAll();

  //释放
  DestroyDecoder();
  render->Destroy();
  decodeQueue.Destroy();

  videoState = CCVideoState::NotOpen;
  CallPlayerEventCallback(PLAYER_EVENT_CLOSED);
}
void CCVideoPlayer::DoSeekVideo() {

  //先停止序列
  StopDecoderThread();
  render->Stop();

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
    avcodec_flush_buffers(videoCodecContext);//清空缓存数据
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

  //启动线程
  LOGD("Start all for seek");

  decoderAudioFinish = audioIndex == -1;
  decoderVideoFinish = false;

  if (videoState == CCVideoState::Playing) {
    StartDecoderThread();
    render->Start(false);
  }
  else {
    StartDecoderThread(true);
    render->Start(true);
  }
  playerSeeking = 0;
}

//播放器公共方法
//**************************

bool CCVideoPlayer::OpenVideo(const char* filePath) {

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
  videoState = CCVideoState::Loading;

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

  if (videoState == CCVideoState::NotOpen || videoState == CCVideoState::Failed) {
    LOGE("Can not close video because video not load");
    return false;
  }
  if (playerClose == 0) {
    playerClose = 1;
    return true;
  }
  return true;
}
void CCVideoPlayer::SetVideoState(CCVideoState newState) {
  if (videoState == newState)
    return;

  switch (newState) {
  case CCVideoState::NotOpen: CloseVideo(); break;
  case CCVideoState::Playing:
    if (GetVideoPos() > GetVideoLength() - 500)
      SetVideoPos(0);
    StartAll();
    break;
  case CCVideoState::Paused: StopAll(); break;
  case CCVideoState::Ended:
  case CCVideoState::Failed:
  case CCVideoState::Loading:
    LOGEF("Bad state %d, this state can only get.", newState);
    SetLastError(VIDEO_PLAYER_ERROR_STATE_CAN_ONLY_GET, "");
    return;
  }

  LOGDF("SetVideoState : %s", CCVideoStateToString(newState));
}
void CCVideoPlayer::SetVideoPos(int64_t pos) {
  if (playerSeeking == 0) {
    playerSeeking = 1;
    seekDest = externalData.StartTime + pos;
    LOGDF("SetVideoPos : %ld/%ld", seekDest, GetVideoLength());
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
CCVideoState CCVideoPlayer::GetVideoState() { return videoState; }
int64_t CCVideoPlayer::GetVideoLength() {
  if (!formatContext) {
    SetLastError(VIDEO_PLAYER_ERROR_NOT_OPEN, "Video not open");
    LOGE("Video not open");
    return 0;
  }
  return (int64_t)(formatContext->duration / (double)AV_TIME_BASE * (double)1000); //ms
}
void CCVideoPlayer::SetVideoVolume(int vol) { render->SetVolume(vol); }
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
  videoState = CCVideoState::Playing;

  startAllLock.lock();

  decoderAudioFinish = audioIndex == -1;
  decoderVideoFinish = false;
  StartDecoderThread();
  render->Start(false);

  startAllLock.unlock();
}
void CCVideoPlayer::StopAll() {
  if (videoState == CCVideoState::Paused || videoState == CCVideoState::Ended)
    return;
  videoState = CCVideoState::Paused;

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
      LOGEF("Failed to open input file, error : %s", errBuf);
      SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("Failed to open input file, error : %s", errBuf).c_str());
    }
    return false;
  }
  //为分配的AVFormatContext 结构体中填充数据
  if (avformat_find_stream_info(formatContext, nullptr) < 0) {
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, "Failed to read the input video stream information");
    LOGE("Failed to read the input video stream information");
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
      LOGDF("video stream : %d", i);
    } else if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      LOGDF("audio stream : %d", i);
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
    LOGE("Not found video stream!");
    return false;
  }

  LOGDF("formatContext->nb_streams : %d", formatContext->nb_streams);
  LOGDF("audioIndex : %d", audioIndex);
  LOGDF("videoIndex : %d", videoIndex);

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
    LOGE("Not find video decoder");
    SetLastError(VIDEO_PLAYER_ERROR_VIDEO_NOT_SUPPORT, "Not find video decoder");
    return false;
  }

  //通过解码器分配(并用  默认值   初始化)一个解码器context
  videoCodecContext = avcodec_alloc_context3(videoCodec);
  if (videoCodecContext == nullptr) {
    LOGE("avcodec_alloc_context3 for videoCodecContext failed");
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, "avcodec_alloc_context3 for videoCodecContext failed");
    return false;
  }

  //更具指定的编码器值填充编码器上下文
  ret = avcodec_parameters_to_context(videoCodecContext, codecParameters);
  if (ret < 0) {
    LOGEF("avcodec_parameters_to_context videoCodecContext failed : %d", ret);
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_parameters_to_context videoCodecContext failed : %d", ret).c_str());
    return false;
  }
  ret = avcodec_open2(videoCodecContext, videoCodec, nullptr);
  //通过所给的编解码器初始化编解码器上下文
  if (ret < 0) {
    LOGEF("avcodec_open2 videoCodecContext failed : %d", ret);
    SetLastError(VIDEO_PLAYER_ERROR_AV_ERROR, StringHelper::FormatString("avcodec_open2 videoCodecContext failed : %d", ret).c_str());
    return false;
  }

  //初始化音频解码器
  //***********************************

  if (audioIndex > -1) {

    codecParameters = formatContext->streams[audioIndex]->codecpar;
    audioCodec = avcodec_find_decoder(codecParameters->codec_id);
    if (audioCodec == nullptr) {
      LOGW("Not find audio decoder");
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
      LOGWF("avcodec_parameters_to_context audioCodecContext failed : %d", ret);
      goto AUDIO_INIT_DONE;
    }
    ret = avcodec_open2(audioCodecContext, audioCodec, nullptr);
    //通过所给的编解码器初始化编解码器上下文
    if (ret < 0) {
      LOGWF("avcodec_open2 audioCodecContext failed : %d", ret);
      goto AUDIO_INIT_DONE;
    }

  }
  else audioCodecContext = nullptr;
AUDIO_INIT_DONE:

  LOGDF("audioCodecContext->codec_id : %d", audioCodecContext ? audioCodecContext->codec_id : 0);
  LOGDF("videoCodecContext->codec_id : %d", videoCodecContext->codec_id);

  externalData.StartTime = formatContext->start_time * 1000 / AV_TIME_BASE;

  externalData.AudioCodecContext = audioCodecContext;
  externalData.VideoCodecContext = videoCodecContext;
  externalData.FormatContext = formatContext;

  decodeState = CCDecodeState::Ready;
  return true;
}
bool CCVideoPlayer::DestroyDecoder() {

  if (decodeState < CCDecodeState::Ready) {
    SetLastError(VIDEO_PLAYER_ERROR_NOR_INIT, "Decoder not init");
    LOGE("Decoder not init");
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

  decodeState = CCDecodeState::NotInit;
  return true;
}

//解码器线程控制
//**************************

void CCVideoPlayer::StartDecoderThread(bool isStartBySeek) {

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
  decodeState = CCDecodeState::Ready;
  if (decoderWorkerThread) {
    delete decoderWorkerThread;
    decoderWorkerThread = nullptr;
  }
  if (decoderVideoThread) {
    delete decoderVideoThread;
    decoderVideoThread = nullptr;
  }
  if (decoderAudioThread) {
    delete decoderAudioThread;
    decoderAudioThread = nullptr;
  }
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
        videoState = CCVideoState::Ended;

        CallPlayerEventCallback(PLAYER_EVENT_PLAY_DONE);
        LOGIF("decodeState -> Finished pos: %d dur: %d", pos, dur);
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
  LOGIF("DecoderWorkerThread : Start: [%s]", CCDecodeStateToString(decodeState));

  while (decodeState == CCDecodeState::Decoding) {

    auto maxMaxRenderQueueSize =  InitParams.MaxRenderQueueSize;
    if (decodeQueue.AudioQueueSize() > maxMaxRenderQueueSize || decodeQueue.VideoQueueSize() > maxMaxRenderQueueSize) {
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
    }
    else if (ret == AVERROR_EOF) {
      //读取完成，但是可能还没有播放完成
      if (
        decodeQueue.AudioQueueSize() == 0
        && decodeQueue.VideoQueueSize() == 0
      ) {
        decodeQueue.ReleasePacket(avPacket);
        decodeState = CCDecodeState::Finish;
        break;
      }
      else
        decodeQueue.ReleasePacket(avPacket);
    }
    else {
      LOGEF("DecoderWorkerThread", "av_read_frame failed : %d", ret);
      decodeQueue.ReleasePacket(avPacket);
      decodeState = CCDecodeState::FinishedWithError;
      break;
    }
  }

  LOGIF("DecoderWorkerThread : End [%s]", CCDecodeStateToString(decodeState));
  return nullptr;
}
void* CCVideoPlayer::DecoderVideoThread() {
  //视频解码线程
  LOGIF("DecoderVideoThread : Start: [%s]", CCDecodeStateToString(decodeState));

  int ret;
  AVPacket* packet;
  AVFrame* frame;
  while (decodeState >= CCDecodeState::Decoding) {

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

    //延时
    if (decodeQueue.VideoFrameQueueSize() > 50u) {
      av_usleep(1000 * 100);
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
  return nullptr;
}
void* CCVideoPlayer::DecoderAudioThread() {
  //音频解码线程
  LOGIF("DecoderAudioThread : Start: [%s]", CCDecodeStateToString(decodeState));

  int ret;
  AVPacket* packet;
  AVFrame* frame;
  while (decodeState >= CCDecodeState::Decoding) {

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

    if (decodeQueue.AudioFrameQueueSize() > 20u) {
      av_usleep(1000 * 20);
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
  return nullptr;
}

//同步渲染

void CCVideoPlayer::SyncRender()
{
  return render->SyncRender();
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
  LOGEF("CCVideoPlayer Error: %s", StringHelper::UnicodeToAnsi(errmsg).c_str());
  lastErrorMessage = errmsg ? errmsg : L"";
  lastErrorCode = code;
}
void CCVideoPlayer::SetLastError(int code, const char* errmsg)
{
  LOGEF("CCVideoPlayer Error: %s", errmsg);
  lastErrorMessage = errmsg ? StringHelper::AnsiToUnicode(errmsg) : L"";
  lastErrorCode = code;
}
const wchar_t* CCVideoPlayer::GetLastErrorMessage()
{
  return lastErrorMessage.c_str();
}










