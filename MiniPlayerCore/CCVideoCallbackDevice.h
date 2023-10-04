#pragma once
#include "CCVideoDevice.h"
#include "CCVideoPlayer.h"

class CCVideoCallbackDevice : public CCVideoDevice
{
public:
  CCVideoCallbackDevice(CCVideoPlayerExternalData* data);

  void Destroy();
  uint8_t* Lock(uint8_t* src, int srcStride, int* destStride, int64_t pts);
  void Unlock();
  void Dirty();

  void Pause();
  void Reusme();
  void Reset();

private:
  CCVideoPlayerExternalData* data;

  void CallSimpleMessage(int message);
};

