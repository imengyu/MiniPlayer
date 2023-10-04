#include "pch.h"
#include "CCVideoCallbackDevice.h"

CCVideoCallbackDevice::CCVideoCallbackDevice(CCVideoPlayerExternalData* data)
{
  this->data = data;
}

void CCVideoCallbackDevice::Destroy()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_DESTROY);
}
uint8_t* CCVideoCallbackDevice::Lock(uint8_t* src, int srcStride, int* destStride, int64_t pts)
{
  CCVideoPlayerCallbackDeviceData data;
  data.type = PLAYER_EVENT_RDC_TYPE_LOCK;
  data.src = src;
  data.srcStride = srcStride;
  data.destStride = 0;
  data.dest = nullptr;
  data.pts = pts; 
  this->data->Player->CallPlayerEventCallback(PLAYER_EVENT_RENDER_DATA_CALLBACK, &data);
  if (data.destStride)
    *destStride = data.destStride;
  return  data.dest;
}

void CCVideoCallbackDevice::Unlock()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_UNLOCK);
}
void CCVideoCallbackDevice::Dirty()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_DIRTY);
}

void CCVideoCallbackDevice::Pause()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_UNLOCK);
}
void CCVideoCallbackDevice::Reusme()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_PAUSE);
}
void CCVideoCallbackDevice::Reset()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_RESET);
}

void CCVideoCallbackDevice::CallSimpleMessage(int message)
{
  CCVideoPlayerCallbackDeviceData data;
  data.type = message;
  this->data->Player->CallPlayerEventCallback(PLAYER_EVENT_RENDER_DATA_CALLBACK, &data);
}
