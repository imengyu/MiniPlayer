//
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

  destChannels = audioDeviceDefaultFormatInfo.channels;

  AVChannelLayout dst_ch_layout;
  dst_ch_layout.nb_channels = audioDeviceDefaultFormatInfo.channels;
  dst_ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
  dst_ch_layout.u.mask = AV_CH_LAYOUT_STEREO;

  // 配置输入/输出通道类型
  av_opt_set_chlayout(swrContext, "in_channel_layout", &externalData->AudioCodecContext->ch_layout, 0);
  av_opt_set_chlayout(swrContext, "out_channel_layout", &dst_ch_layout, 0);
  // 配置输入/输出采样率
  av_opt_set_int(swrContext, "in_sample_rate", externalData->AudioCodecContext->sample_rate, 0);
  av_opt_set_int(swrContext, "out_sample_rate", audioDeviceDefaultFormatInfo.sampleRate, 0);
  // 配置输入/输出数据格式
  av_opt_set_sample_fmt(swrContext, "in_sample_fmt", externalData->AudioCodecContext->sample_fmt, 0);
  av_opt_set_sample_fmt(swrContext, "out_sample_fmt", audioDeviceDefaultFormatInfo.fmt, 0);

  int error = swr_init(swrContext);
  if (error != 0) {
    char str[50];
    LOGEF("swr_init failed: %s (%d)", av_make_error_string(str, 50, error), error);
    return false;
  }
  
  // 重采样后，每个通道包含的采样数
  // acc默认为1024，重采样后可能会变化
  destNbSample = (int)av_rescale_rnd(
    ACC_NB_SAMPLES,
    audioDeviceDefaultFormatInfo.sampleRate,
    externalData->AudioCodecContext->sample_rate, 
    AV_ROUND_UP
  );

  // 重采样后一帧数据的大小
  destDataSize = (size_t)av_samples_get_buffer_size(
    &destLinesize,
    audioDeviceDefaultFormatInfo.channels,
    destNbSample,
    audioDeviceDefaultFormatInfo.fmt, 
    1
  ); 
  
  destDataSizePerSample = av_get_bytes_per_sample(audioDeviceDefaultFormatInfo.fmt);
  destDataSizeOne = (size_t)av_samples_get_buffer_size(
    &destLinesize,
    audioDeviceDefaultFormatInfo.channels,
    1,
    audioDeviceDefaultFormatInfo.fmt,
    1
  );

  av_samples_alloc_array_and_samples(
    &audioOutBuffer, 
    &destLinesize,
    audioDeviceDefaultFormatInfo.channels, 
    destNbSample,
    audioDeviceDefaultFormatInfo.fmt,
    0
  );

  fopen_s(&out_file, "out.pcm", "wb");
  audioDevice->SetOnCopyDataCallback(RenderAudioBufferDataStub);
  audioDevice->SetOnRequestDataSizeCallback(RenderAudioRequestFrameSizeStub);

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
  if (audioOutBuffer != nullptr) {
    av_freep(audioOutBuffer);
    audioOutBuffer = nullptr;
  }
  if (audioDevice != nullptr) {
    audioDevice->Stop();
    audioDevice->Destroy();
    audioDevice = nullptr;
  }
  if (videoDevice != nullptr) {
    videoDevice->Destroy();
    videoDevice = nullptr;
  }
  
  fclose(out_file);
}
void CCPlayerRender::Stop() {
  if (status != CCRenderState::NotRender) {
    status = CCRenderState::NotRender;

    if (renderVideoThread) {
      delete renderVideoThread;
      renderVideoThread = nullptr;
    }
    if (renderAudioThread) {
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

  if (!externalData->InitParams->SyncRender && !renderVideoThread) {
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

void CCPlayerRender::SyncRender()
{
  RenderVideoThreadWorker();
}

bool CCPlayerRender::RenderVideoThreadWorker() {
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
    return true;
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
          av_usleep((uint32_t)((delays * 2) * 1000000));
        }
        else {
          av_usleep((uint32_t)((delays + diff) * 1000000));
        }
      }
      else {
        if (diff >= 0.55) {
          externalData->DecodeQueue->ReleaseFrame(frame);
          int count = externalData->DecodeQueue->VideoDrop(currentAudioClock);
          //LOGDF("Sync: drop video pack: %d", count);
          return true;
        }
        else {
          av_usleep((uint32_t)1000);
        }
      }
    }
    else {
      //正常播放
      av_usleep((uint32_t)(delays * 1000000));
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

    videoDevice->Render(outFrame, curVideoPts);

    av_frame_unref(outFrame);
  }

  externalData->DecodeQueue->ReleaseFrame(frame);

  if (status == CCRenderState::RenderingToSeekPos) {
    curAudioPts = curVideoPts;
    currentSeekToPosFinished = true;
    LOGD("RenderVideoThread SeekToPosFinished");
    return false;
  }

  return true;
}
void* CCPlayerRender::RenderVideoThreadStub(void* param) {
  void* result = ((CCPlayerRender*)param)->RenderVideoThread();
  ((CCPlayerRender*)param)->renderVideoThread = 0;
  return result;
}
void* CCPlayerRender::RenderVideoThread() {
  LOGDF("RenderVideoThread Start [%s]", CCRenderStateToString(status));

  while (status == CCRenderState::Rendering || status == CCRenderState::RenderingToSeekPos) {
    if (!RenderVideoThreadWorker())
      break;
  }

  LOGD("RenderVideoThread End");
  return nullptr;
}

bool CCPlayerRender::RenderAudioBufferDataStub(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len, DWORD sample) {
  return dynamic_cast<CCPlayerRender*>(instance)->RenderAudioBufferData(buf, buf_len, sample);
}
bool CCPlayerRender::RenderAudioRequestFrameSizeStub(CSoundDeviceHoster* instance, DWORD maxSample, DWORD* sample)
{
  return dynamic_cast<CCPlayerRender*>(instance)->RenderAudioRequestFrameSize(maxSample, sample);
}
bool CCPlayerRender::RenderAudioRequestFrameSize(DWORD maxSample, DWORD* sample) {
  if (status == CCRenderState::Rendering) {

    auto audioDeviceDefaultFormatInfo = audioDevice->RequestDeviceDefaultFormatInfo();

    //有遗留数据，需要先取用
    if (destLeaveSamples > 0) {
      auto thisSample = min(destLeaveSamples, maxSample);
      *sample = thisSample;
      destLeaveSamples -= thisSample;
      destDataOffset += destDataSizeOne * thisSample;
      return true;
    }

    AVFrame* frame = externalData->DecodeQueue->AudioFrameDequeue();
    if (frame == nullptr) {
      *sample = 0;
      return false;
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
      destDataSamples = swr_convert(
        swrContext,
        audioOutBuffer,
        destNbSample,
        (const uint8_t**)frame->data,
        frame->nb_samples
      );

      if (destDataSamples > 0) {
        *sample = min(destDataSamples, maxSample);
        if (destDataSamples > maxSample) {
          destLeaveSamples = destDataSamples - maxSample;
          destDataOffset = destDataSizeOne * destLeaveSamples;
        }
        else {
          destLeaveSamples = 0;
          destDataOffset = 0;
        }

        //pts时间+当前帧播放需要的时间
        currentAudioClock += destDataSamples / ((double)(audioDeviceDefaultFormatInfo.sampleRate * destChannels));
      }
    }

    externalData->DecodeQueue->ReleaseFrame(frame);
  }
  return true;
}
bool CCPlayerRender::RenderAudioBufferData(LPVOID buf, DWORD buf_len, DWORD sample) 
{
  if (buf) {
    memcpy_s(buf, buf_len, (void*)(audioOutBuffer[0] + destDataOffset), min(buf_len, (DWORD)destLinesize));
    fwrite((void*)(audioOutBuffer[0] + destDataOffset), destDataSizePerSample * 2, sample, out_file);
  }
  return true;
}



