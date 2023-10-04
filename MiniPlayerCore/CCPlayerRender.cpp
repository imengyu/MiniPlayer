﻿//
// Created by roger on 2020/12/22.
//

#include "pch.h"
#include "CCPlayerRender.h"
#include "CCVideoPlayer.h"
#include "CSoundDevice.h"
#include "CCVideoDevice.h"
#include "CCVideoCallbackDevice.h"
#include "Logger.h"

bool CCPlayerRender::Init(CCVideoPlayerExternalData* data) {
  externalData = data;

  audioDevice = CreateAudioDevice(data);
  videoDevice = CreateVideoDevice(data);

  //初始化SwsContext
  swsContext = sws_getContext(
    data->VideoCodecContext->width,   //原图片的宽
    data->VideoCodecContext->height,  //源图高
    data->VideoCodecContext->pix_fmt, //源图片format
    data->InitParams->DestWidth,  //目标图的宽
    data->InitParams->DestHeight,  //目标图的高
    (AVPixelFormat)data->InitParams->DestFormat,
    SWS_BICUBIC,
    nullptr, nullptr, nullptr
  );

  if (swsContext == nullptr) {
    LOGEF("Get swsContext failed");
    return false;
  }

  //初始化 swrContext
  swrContext = swr_alloc();
  if (swrContext == nullptr) {
    LOGEF("Alloc swrContext failed");
    return false;
  }

  auto audioDeviceDefaultFormatInfo = audioDevice->RequestDeviceDefaultFormatInfo();
  if (audioDeviceDefaultFormatInfo.sampleRate == 0)
    audioDeviceDefaultFormatInfo.sampleRate = externalData->AudioCodecContext->sample_rate;
  if (audioDeviceDefaultFormatInfo.fmt == 0)
    audioDeviceDefaultFormatInfo.fmt = externalData->AudioCodecContext->sample_fmt;

  int distChannels = 0;
  switch (audioDeviceDefaultFormatInfo.channels)
  {
  case 1: distChannels = AV_CH_LAYOUT_MONO; break;
  case 2: distChannels = AV_CH_LAYOUT_STEREO; break;
  case 0:
  default: break;
  }

  // 配置输入/输出通道类型
  av_opt_set_chlayout(swrContext, "in_channel_layout", &externalData->AudioCodecContext->ch_layout, 0);
  av_opt_set_int(swrContext, "out_channel_layout", distChannels, 0);
  // 配置输入/输出采样率
  av_opt_set_int(swrContext, "in_sample_rate", externalData->AudioCodecContext->sample_rate, 0);
  av_opt_set_int(swrContext, "out_sample_rate", audioDeviceDefaultFormatInfo.sampleRate, 0);
  // 配置输入/输出数据格式
  av_opt_set_sample_fmt(swrContext, "in_sample_fmt", externalData->AudioCodecContext->sample_fmt, 0);
  av_opt_set_sample_fmt(swrContext, "out_sample_fmt", audioDeviceDefaultFormatInfo.fmt, 0);

  swr_init(swrContext);

  // 重采样后一个通道采样数
  destNbSample = (int)av_rescale_rnd(
    ACC_NB_SAMPLES,
    audioDeviceDefaultFormatInfo.sampleRate,
    externalData->AudioCodecContext->sample_rate, 
    AV_ROUND_UP
  );
  // 重采样后一帧数据的大小
  destDataSize = (size_t)av_samples_get_buffer_size(
    nullptr,
    audioDeviceDefaultFormatInfo.channels,
    destNbSample,
    audioDeviceDefaultFormatInfo.fmt, 
    1
  );

  audioOutBuffer[0] = (uint8_t*)malloc(destDataSize);
  audioDevice->SetOnCopyDataCallback(RenderAudioBufferDataStub);

  return true;
}
void CCPlayerRender::Destroy() {
  if (outFrame != nullptr)
    av_frame_free(&outFrame);
  if (swsContext != nullptr) {
    sws_freeContext(swsContext);
    swsContext = nullptr;
  }
  if (swrContext != nullptr) {
    swr_free(&swrContext);
  }
  if (audioOutBuffer[0] != nullptr) {
    free(audioOutBuffer[0]);
    audioOutBuffer[0] = nullptr;
  }
  if (audioDevice != nullptr) {
    audioDevice->Destroy();
    audioDevice = nullptr;
  }
  if (videoDevice != nullptr) {
    videoDevice->Destroy();
    videoDevice = nullptr;
  }
}
void CCPlayerRender::Stop() {
  if (status != CCRenderState::NotRender) {
    status = CCRenderState::NotRender;

    if (renderVideoThread) {
      renderVideoThread->join();
      delete renderVideoThread;
      renderVideoThread = nullptr;
    }
    if (renderAudioThread) {
      renderAudioThread->join();
      delete renderAudioThread;
      renderAudioThread = nullptr;
    }

    audioDevice->Stop();
    videoDevice->Pause();
  }
}
void CCPlayerRender::Start(bool isStartBySeek) {

  status = isStartBySeek ? CCRenderState::RenderingToSeekPos : CCRenderState::Rendering;

  if (!isStartBySeek)
    audioDevice->Start();
  videoDevice->Reusme();

  if (!renderVideoThread) {
    renderVideoThread = new std::thread(RenderVideoThreadStub, this);
    renderVideoThread->detach();
  }
}
void CCPlayerRender::Reset() {
  audioDevice->Reset();
  videoDevice->Reset();

  curVideoDts = 0;
  curVideoPts = 0;
  curAudioDts = 0;
  curAudioPts = 0;
}

void CCPlayerRender::SetVolume(int vol) {
  if (audioDevice)
    audioDevice->SetVolume(-1, vol / 100.0f);
}
void CCPlayerRender::SetLastError(int code, const wchar_t* errmsg)
{
  externalData->Player->SetLastError(code, errmsg);
}

CCVideoDevice* CCPlayerRender::CreateVideoDevice(CCVideoPlayerExternalData* data) {
  if (data->InitParams->UseRenderCallback)
    return new CCVideoCallbackDevice(data);
  return new CCVideoDevice();
}
CSoundDevice* CCPlayerRender::CreateAudioDevice(CCVideoPlayerExternalData* data) {
  auto device = new CSoundDevice(this);
  return device;
}

void* CCPlayerRender::RenderVideoThreadStub(void* param) {
  void* result = ((CCPlayerRender*)param)->RenderVideoThread();
  ((CCPlayerRender*)param)->renderVideoThread = 0;
  return result;
}
void* CCPlayerRender::RenderVideoThread() {
  LOGDF("RenderVideoThread Start [%s]", CCRenderStateToString(status));

  while (status == CCRenderState::Rendering || status == CCRenderState::RenderingToSeekPos) {

    if (outFrame == nullptr || outFrameDestFormat != externalData->InitParams->DestFormat
      || outFrameDestWidth != externalData->InitParams->DestWidth
      || outFrameDestHeight != externalData->InitParams->DestHeight) {

      if (outFrame != nullptr)
        av_frame_free(&outFrame);

      outFrameDestFormat = (AVPixelFormat)externalData->InitParams->DestFormat;
      outFrameDestWidth = externalData->InitParams->DestWidth;
      outFrameDestHeight = externalData->InitParams->DestHeight;

      outFrame = av_frame_alloc();
      outFrameBufferSize = (size_t)av_image_get_buffer_size(outFrameDestFormat, outFrameDestWidth, outFrameDestHeight, 1);
      outFrameBuffer = (uint8_t*)av_malloc(outFrameBufferSize);
    }

    double frame_delays = 1.0 / externalData->CurrentFps;
    AVFrame* frame = externalData->DecodeQueue->VideoFrameDequeue();

    if (frame == nullptr) {
      av_usleep((int64_t)(100000));
      continue;
    }

    //时钟
    AVRational time_base = externalData->VideoTimeBase;
    if (frame->best_effort_timestamp == AV_NOPTS_VALUE)
      currentVideoClock = frame->pts * av_q2d(time_base);
    else
      currentVideoClock = frame->best_effort_timestamp * av_q2d(time_base);

    curVideoDts = frame->pkt_dts;
    curVideoPts = frame->pts;

    //延迟计算
    double extra_delay = frame->repeat_pict / (2 * externalData->CurrentFps);
    double delays = extra_delay + frame_delays;

    //与音频同步
    if (status != CCRenderState::RenderingToSeekPos) {
      if (externalData->AudioCodecContext != nullptr) {
        //音频与视频的时间差
        double diff = fabs(currentVideoClock - currentAudioClock);
        //LOGDF("Sync: diff: %f, v/a %f/%f", diff, currentVideoClock, currentAudioClock);
        if (currentVideoClock > currentAudioClock) {
          if (diff > 1) {
            av_usleep((int64_t)((delays * 2) * 1000000));
          }
          else {
            av_usleep((int64_t)((delays + diff) * 1000000));
          }
        }
        else {
          if (diff >= 0.55) {
            externalData->DecodeQueue->ReleaseFrame(frame);
            /*int count = */externalData->DecodeQueue->VideoDrop(currentAudioClock);
            //LOGDF("Sync: drop video pack: %d", count);
            continue;
          }
          else {
            av_usleep((int64_t)1000);
          }
        }
      }
      else {
        //正常播放
        av_usleep((int64_t)(delays * 1000000));
      }
    }

    {
      memset(outFrameBuffer, 0, outFrameBufferSize);

      //更具指定的数据初始化/填充缓冲区
      av_image_fill_arrays(
        outFrame->data, 
        outFrame->linesize, 
        outFrameBuffer, 
        outFrameDestFormat,
        outFrameDestWidth, 
        outFrameDestHeight, 
        1
      );
      //转码
      sws_scale(
        swsContext, 
        (const uint8_t* const*)frame->data, 
        frame->linesize, 
        0,
        frame->height,
        outFrame->data, 
        outFrame->linesize
      );

      uint8_t* src = outFrame->data[0];
      int srcStride = outFrame->linesize[0];
      int destStride = 0;
      uint8_t* target = videoDevice->Lock(src, srcStride, &destStride, curVideoPts);
      if (target && src) {
        for (int i = 0, c = externalData->VideoCodecContext->height; i < c; i++)
          memcpy(target + i * destStride, src + i * srcStride, srcStride);
        videoDevice->Dirty();
      }
      else if (!src) {
        LOGWF("Frame %ld scale failed", frame->pts);
      }
      videoDevice->Unlock();

      av_frame_unref(outFrame);
    }

    externalData->DecodeQueue->ReleaseFrame(frame);

    if (status == CCRenderState::RenderingToSeekPos) {
      curAudioPts = curVideoPts;
      currentSeekToPosFinished = true;
      LOGD("RenderVideoThread SeekToPosFinished");
      break;
    }
  }

  LOGD("RenderVideoThread End");
  return nullptr;
}

bool CCPlayerRender::RenderAudioBufferDataStub(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len) {
  return dynamic_cast<CCPlayerRender*>(instance)->RenderAudioBufferData((uint8_t**)buf, &buf_len);
}
bool CCPlayerRender::RenderAudioBufferData(uint8_t** buf, DWORD* len) {
  if (status == CCRenderState::Rendering) {

    AVFrame* frame = nullptr;
    int noneFrameTick = 0;

    while (frame == nullptr) {
      av_usleep((int64_t)(10000));
      frame = externalData->DecodeQueue->AudioFrameDequeue();
      if (noneFrameTick < 32) noneFrameTick++;
      else return false;
    }

    //时钟
    AVRational time_base = externalData->AudioTimeBase;
    if (frame->best_effort_timestamp == AV_NOPTS_VALUE)
      currentAudioClock = 0;
    else
      currentAudioClock = frame->best_effort_timestamp * av_q2d(time_base);

    curAudioDts = frame->pkt_dts;
    curAudioPts = frame->pts;

    {
      // 转换，返回每个通道的样本数
      int samples = swr_convert(swrContext, audioOutBuffer, (int)destDataSize / 2,
        (const uint8_t**)frame->data, frame->nb_samples);
      if (samples > 0) {
        *buf = audioOutBuffer[0];
        *len = destDataSize;

        //pts时间+当前帧播放需要的时间
        currentAudioClock += samples / ((double)(audioDevice->RequestDeviceDefaultFormatInfo().sampleRate * 2 * 2));
      }

      //audioDevice->Write(audioOutBuffer[0], (size_t) destDataSize, 0);

      //double frame_delays = 1.0 / externalData->CurrentFps;
      //double extra_delay = frame->repeat_pict / (2 * externalData->CurrentFps);
      //double delays = (extra_delay + frame_delays) / 2;
      //av_usleep((int64_t)(delays * 1000000));
    }

    externalData->DecodeQueue->ReleaseFrame(frame);
  }
  return true;
}



