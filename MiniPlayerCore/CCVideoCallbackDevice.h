#pragma once
#include "CCVideoDevice.h"
#include "CCVideoPlayer.h"

class CCVideoCallbackDevice : public CCVideoDevice
{
public:
  CCVideoCallbackDevice(CCVideoPlayerExternalData* data);

  void Destroy();
  void Render(AVFrame* frame, int64_t pts);

  void Pause();
  void Reusme();
  void Reset();

private:
  CCVideoPlayerExternalData* data;

  void CallSimpleMessage(int message);
};

