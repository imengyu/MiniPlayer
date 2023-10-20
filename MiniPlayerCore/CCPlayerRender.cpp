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

  //申请音频缓冲区大小
  //为了可以最快速将所有数据拷贝到WASAPI，这里还有一个遗留缓冲
  audioOutLeave = new CAppendBuffer(destDataSize * 2);
  av_samples_alloc_array_and_samples(
    &audioOutBuffer,
    &destLinesize,
    audioDeviceDefaultFormatInfo.channels,
    destNbSample,
    audioDeviceDefaultFormatInfo.fmt,
    0
  );

  audioDevice->SetOnCopyDataCallback(RenderAudioBufferDataStub);


#if _DEBUG && 0
  fopen_s(&out_file, "out.pcm", "wb");
#endif

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
  if (audioOutLeave != nullptr) {
    delete audioOutLeave;
    audioOutLeave = nullptr;
  }
  if (audioOutBuffer != nullptr) {
    av_freep(audioOutBuffer);
    audioOutBuffer[0] = nullptr;
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
  if (out_file) {
    fclose(out_file);
    out_file = nullptr;
  }
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
void CCPlayerRender::Start() {

  status = CCRenderState::Rendering;

  audioDevice->Start();
  audioDevice->SetVolume(-1, currentVolume / 100.0f);

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
  currentVolume = vol;
  if (audioDevice)
    audioDevice->SetVolume(-1, vol / 100.0f);
}
void CCPlayerRender::SetLastError(int code, const wchar_t* errmsg)
{
  externalData->Player->SetLastError(code, errmsg);
}

void CCPlayerRender::UpdateDestSize()
{
  if (status == CCRenderState::Rendering) {
    if (swsContext != nullptr)
      sws_freeContext(swsContext);
    swsContext = sws_getContext(
      externalData->VideoCodecContext->width,   //原图片的宽
      externalData->VideoCodecContext->height,  //源图高
      externalData->VideoCodecContext->pix_fmt, //源图片format
      externalData->InitParams->DestWidth,  //目标图的宽
      externalData->InitParams->DestHeight,  //目标图的高
      (AVPixelFormat)externalData->InitParams->DestFormat,
      SWS_BICUBIC,
      nullptr, nullptr, nullptr
    );
  }
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

CCVideoPlayerCallbackDeviceData* CCPlayerRender::SyncRenderStart()
{
  if (currentFrame)
    return nullptr;
  if (!RenderVideoThreadWorker(true))
    memset(&syncRenderData, 0, sizeof(syncRenderData));
  return &syncRenderData;
}
void CCPlayerRender::SyncRenderEnd() {
  av_frame_unref(outFrame);

  if (currentFrame) {
    externalData->DecodeQueue->ReleaseFrame(currentFrame);
    currentFrame = nullptr;
  }
}

bool CCPlayerRender::RenderVideoThreadWorker(bool sync) {
  if (outFrame == nullptr 
    || outFrameDestFormat != externalData->InitParams->DestFormat
    || outFrameDestWidth != externalData->InitParams->DestWidth
    || outFrameDestHeight != externalData->InitParams->DestHeight
    ) 
  {
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
  currentFrame = externalData->DecodeQueue->VideoFrameDequeue();

  if (currentFrame == nullptr) {
    if (!sync) {
      av_usleep((int64_t)(100000));
      LOGD("Empty video frame");
    }
    return false;
  }

  //时钟
  AVRational time_base = externalData->VideoTimeBase;
  if (currentFrame->best_effort_timestamp == AV_NOPTS_VALUE)
    currentVideoClock = currentFrame->pts * av_q2d(time_base);
  else
    currentVideoClock = currentFrame->best_effort_timestamp * av_q2d(time_base);

  curVideoDts = currentFrame->pkt_dts;
  curVideoPts = currentFrame->pts;

  //延迟计算
  double extra_delay = currentFrame->repeat_pict / (2 * externalData->CurrentFps);
  double delays = extra_delay + frame_delays;
  
  //与音频同步
  if (externalData->AudioCodecContext != nullptr && externalData->InitParams->SyncVideoAndAudio) {
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
        externalData->DecodeQueue->ReleaseFrame(currentFrame);
        currentFrame = nullptr;
        int count = externalData->DecodeQueue->VideoDrop(currentAudioClock);
        LOGDF("Sync: drop video pack: %d", count);
        return false;
      }
      else {
        av_usleep((uint32_t)1000);
      }
    }
  }
  else if (!sync) {
    //正常播放
    av_usleep((uint32_t)(delays * 1000000));
  }

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
    (const uint8_t* const*)currentFrame->data,
    currentFrame->linesize,
    0,
    currentFrame->height,
    outFrame->data,
    outFrame->linesize
  );

  if (sync) {
    syncRenderData.data = outFrame->data;
    syncRenderData.linesize = outFrame->linesize;
    syncRenderData.width = outFrameDestWidth;
    syncRenderData.height = outFrameDestHeight;
    syncRenderData.pts = curVideoPts;
    syncRenderData.type = PLAYER_EVENT_RDC_TYPE_RENDER;
    syncRenderData.datasize = av_image_get_buffer_size((AVPixelFormat)externalData->InitParams->DestFormat, outFrameDestWidth, outFrameDestHeight, 1);
  }
  else {
    videoDevice->Render(outFrame, curVideoPts);
    av_frame_unref(outFrame);
    externalData->DecodeQueue->ReleaseFrame(currentFrame);
    currentFrame = nullptr;
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

  while (status == CCRenderState::Rendering)
    RenderVideoThreadWorker(false);

  LOGD("RenderVideoThread End");
  return nullptr;
}

bool CCPlayerRender::RenderAudioBufferDataStub(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len, DWORD sample) {
  return dynamic_cast<CCPlayerRender*>(instance)->RenderAudioBufferData(buf, buf_len, sample);
}
bool CCPlayerRender::RenderAudioBufferData(LPVOID buf, DWORD buf_len, DWORD sample) 
{
  if (buf && buf_len && status == CCRenderState::Rendering) {

    auto buffer = CAppendBuffer(buf, buf_len);
    auto audioDeviceDefaultFormatInfo = audioDevice->RequestDeviceDefaultFormatInfo();

    size_t bufOffset = 0;
    int copyedSamples = 0;

    if (destLeaveSamples > 0) {
      copyedSamples = min(destLeaveSamples, sample);
      //上次有多余数据，但数据少于缓冲区大小，则先拷走数据，然后再解码一帧，填满缓冲区
      buffer.Append(audioOutLeave->Data(), min(buf_len, copyedSamples * destDataSizeOne));
      audioOutLeave->Increase(min(buf_len, copyedSamples * destDataSizeOne));
      destLeaveSamples -= copyedSamples;
    }
    if (destLeaveSamples > 0) {
      //上次有多余数据多余缓冲区大小，这一次就不解码下一帧了
      if (audioOutLeave->GetSpaceSize() > audioOutLeave->GetSize() / 3)
        audioOutLeave->MoveToFirst();
      //LOGDF("Read: data full, leave: %d Clock: %f", destLeaveSamples, currentAudioClock);
      return true;
    }
    else {
      audioOutLeave->Reset();
    }

    //请求帧
    AVFrame* frame = externalData->DecodeQueue->AudioFrameDequeue();
    if (frame == nullptr) {
      memset(buf, 0, buf_len);
      return true;
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
      int destDataSamples = swr_convert(
        swrContext,
        audioOutBuffer,
        destNbSample,
        (const uint8_t**)frame->data,
        frame->nb_samples
      );

      if (destDataSamples > 0) {

        //拷贝新的数据
        auto writeSize = min((int)destDataSamples, (int)sample - copyedSamples) * destDataSizeOne;
        buffer.Append(audioOutBuffer[0], writeSize);
        destLeaveSamples = max(0, destDataSamples - (sample - copyedSamples));

        if (destLeaveSamples > 0) {
          //ffmpeg一帧解码出的数据可能大于缓冲区大小，先保存多出数据下次一起拷贝
          audioOutLeave->AppendNoIncrease(audioOutBuffer[0] + writeSize, destDataSize - writeSize);
        }

        //LOGDF("Read: %d/%d get: %d leave: %d Clock: %f", frame->nb_samples, destDataSamples, sample, destLeaveSamples, currentAudioClock);

        //pts时间+当前帧播放需要的时间
        currentAudioClock += destDataSamples / ((double)(audioDeviceDefaultFormatInfo.sampleRate * destChannels));

#if _DEBUG
        if (out_file)
          fwrite(buffer.FirstData(), 1, buffer.GetFilledSize(), out_file);
#endif
      }
    }

    externalData->DecodeQueue->ReleaseFrame(frame);
  }

  return true;
}



