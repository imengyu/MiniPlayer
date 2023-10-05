#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
extern "C" {
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#define BUFFERNOTIFYSIZE 192000

class CSoundDeviceHoster {
public:
  virtual void NotifyPlayEnd(bool hasError) {}
  virtual void SetLastError(int code, const wchar_t*message) {}

  virtual bool GetShouldReSample() { return false; }
  virtual unsigned long GetSampleRate() { return 0; }
  virtual int GetBitPerSample() { return 0; }
  virtual int GetChannelsCount() { return 0; }
};

typedef bool(*OnCopyDataCallback)(CSoundDeviceHoster* instance, LPVOID buf, DWORD  buf_len);

struct CSoundDeviceDeviceDefaultFormatInfo {
  AVSampleFormat fmt;
  unsigned long sampleRate;
  int channels;
};

class CSoundDevice
{
public:
  CSoundDevice(CSoundDeviceHoster* parent);
  ~CSoundDevice();

  bool Create();
  void Destroy();
  void Reset();
  void Stop();
  bool Start();

  void SetOnCopyDataCallback(OnCopyDataCallback callback);

  float GetVolume(int index);
  void SetVolume(int index, float value);

  UINT32 GetPosition();
  UINT32 GetBufferSize() { return bufferFrameCount; }

  CSoundDeviceDeviceDefaultFormatInfo& RequestDeviceDefaultFormatInfo();

private:
  CSoundDeviceDeviceDefaultFormatInfo deviceDefaultFormatInfo;
  CSoundDeviceHoster* parent;
  bool createSuccess = false;
  bool isStarted = false;
  bool shouldReSample = false;
  float currentVolume[5];
  UINT32 bufferFrameCount = 0;
  UINT32 numFramesPadding = 0;

  OnCopyDataCallback copyDataCallback = nullptr;

  static void PlayerThread(void*p);
private:

  HANDLE hEventCreateDone;
  HANDLE hEventDestroyDone;
  HANDLE hEventResetDone;
  HANDLE hEventDestroy;
  HANDLE hEventPlay;
  HANDLE hEventStop;
  HANDLE hEventLoadData;
  HANDLE hEventReset;
  HANDLE hEventVolumeUpdate;
  HANDLE hEventGetVolume;
  HANDLE hEventGetPadding;

};

