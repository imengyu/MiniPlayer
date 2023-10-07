//
// Created by roger on 2020/12/22.
//

#ifndef VR720_CCPLAYERDEFINE_H
#define VR720_CCPLAYERDEFINE_H

#include "pch.h"
#include "CCVideoPlayerExport.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/jni.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
}

//解码器状态值
//***************************************

enum class CCDecodeState {
  NotInit = 0,
  Preparing = 1,
  Ready = 2,
  Paused = 3,
  Finished = 4,
  FinishedWithError = 5,
  Decoding = 6,
  DecodingToSeekPos = 7,
  Finish = 8,
};

const char* CCDecodeStateToString(CCDecodeState state);

enum class CCRenderState {
  NotRender = 0,
  Rendering = 1,
  RenderingToSeekPos = 2,
};

const char* CCRenderStateToString(CCRenderState state);

const char* CCVideoStateToString(CCVideoState state);

//结构体声明
//***************************************

/**
 * 播放器额外数据
 */
class CCDecodeQueue;
class CCVideoPlayer;
class CCVideoPlayerExternalData {
public:
  CCVideoPlayer* Player = nullptr;
  CCVideoPlayerInitParams* InitParams = nullptr;
  CCDecodeQueue* DecodeQueue = nullptr;
  AVCodecContext* VideoCodecContext = nullptr;
  AVCodecContext* AudioCodecContext = nullptr;
  AVFormatContext* FormatContext = nullptr;
  AVRational VideoTimeBase;
  AVRational AudioTimeBase;
  double CurrentFps = 0;
  int64_t StartTime = 0;
};

//AAC 1024
#define ACC_NB_SAMPLES 1024
#define ACC_SAMPLE_RATE 44100
#define ACC_CHANNELS 2
#define ACC_SAMPLE_FMT AVSampleFormat::AV_SAMPLE_FMT_FLT

#endif //VR720_CCPLAYERDEFINE_H
