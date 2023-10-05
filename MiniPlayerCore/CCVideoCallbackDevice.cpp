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
void CCVideoCallbackDevice::Render(AVFrame* frame, int64_t pts)
{
  CCVideoPlayerCallbackDeviceData data;
  data.type = PLAYER_EVENT_RDC_TYPE_RENDER;
  data.data = frame->data;
  data.linesize = frame->linesize;
  data.pts = pts; 
  this->data->Player->CallPlayerEventCallback(PLAYER_EVENT_RENDER_DATA_CALLBACK, &data);
}

void CCVideoCallbackDevice::Pause()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_PAUSE);
}
void CCVideoCallbackDevice::Reusme()
{
  CallSimpleMessage(PLAYER_EVENT_RDC_TYPE_REUSEME);
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
