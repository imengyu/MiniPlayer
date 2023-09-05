#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#define BUFFERNOTIFYSIZE 192000

class CSoundPlayerImpl;

typedef bool(*OnCopyDataCallback)(CSoundPlayerImpl* instance, LPVOID buf, DWORD  buf_len);

class CSoundDevice
{
public:
  CSoundDevice(CSoundPlayerImpl* parent);
  ~CSoundDevice();

  bool Create();
  void Destroy();
  void Reset();
  void Stop();
  void Start();

  void SetOnCopyDataCallback(OnCopyDataCallback callback);

  float GetVolume(int index);
  void SetVolume(int index, float value);

  UINT32 GetPosition();
  UINT32 GetBufferSize() { return bufferFrameCount; }
private:
  CSoundPlayerImpl* parent;
  bool createSuccess = false;
  bool isStarted = false;
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

