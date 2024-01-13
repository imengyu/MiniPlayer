#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <assert.h>
#include <mutex>
#include "CSoundPlayerExport.h"
extern "C" {
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#define BUFFERNOTIFYSIZE 192000

enum class CSoundDevicePreloadType {
  NoPreload,
  PreloadSame,
  PreloadReload,
};

class CSoundDeviceHoster {
public:
  virtual void NotifyPlayEnd(bool hasError) {}
  virtual CSoundDevicePreloadType PlayAlmostEndAndCheckPrelod() { return CSoundDevicePreloadType::NoPreload; }
  virtual void SetLastError(int code, const wchar_t*message) {}

  virtual const wchar_t* GetDefaultOutputDeviceId() { return nullptr; }

  virtual bool GetShouldReSample() { return false; }
  virtual unsigned long GetSampleRate() { return 0; }
  virtual int GetBitPerSample() { return 0; }
  virtual int GetChannelsCount() { return 0; }
};

typedef bool(*OnCopyDataCallback)(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len, DWORD sample);
typedef bool(*OnRequestDataSizeCallback)(CSoundDeviceHoster* instance, DWORD maxSample, DWORD* nextSample);

struct CSoundDeviceDeviceDefaultFormatInfo {
  AVSampleFormat fmt;
  unsigned long sampleRate;
  int channels;
};

#define MIN_VOL_DB -80

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
  void SetOnRequestDataSizeCallback(OnRequestDataSizeCallback callback);

  float GetVolume(int index);
  void SetVolume(int index, float value);

  int GetPcmDB(const unsigned char* pcmdata, size_t size);
  int* GetCurrentOutputDB() { return currentOutDB; }
  UINT32 GetPosition();
  UINT32 GetBufferSize() { return bufferFrameCount; }


  CSoundDeviceDeviceDefaultFormatInfo& RequestDeviceDefaultFormatInfo();
  static bool GetAllAudioOutDeviceInfo(CSoundDeviceAudioOutDeviceInfo** outList, int* outCount);
  static void DeleteAllAudioOutDeviceInfo(CSoundDeviceAudioOutDeviceInfo** ptr);

private:
  CSoundDeviceDeviceDefaultFormatInfo deviceDefaultFormatInfo;
  CSoundDeviceHoster* parent;
  bool createSuccess = false;
  bool isStarted = false;
  bool shouldReSample = false;
  float currentVolume[5];
  int currentOutDB[2] = { MIN_VOL_DB, MIN_VOL_DB };
  UINT32 bufferFrameCount = 0;
  UINT32 numFramesPadding = 0;
  std::mutex threadLock;
  std::mutex accessStatusLock;

  bool GetThreadStatus();
  void SetThreadStatus(bool newVal);

  OnRequestDataSizeCallback requestDataSizeCallback = nullptr;
  OnCopyDataCallback copyDataCallback = nullptr;

  static void PlayerThread(void*p); 
  
  void HandlePlayError(HRESULT hr);
private:

  HANDLE hEventCreateDone;
  HANDLE hEventDestroyDone;
  HANDLE hEventResetDone;
  HANDLE hEventDestroy;
  HANDLE hEventPlay;
  HANDLE hEventPlayDone;
  HANDLE hEventStop;
  HANDLE hEventStopDone;
  HANDLE hEventLoadData;
  HANDLE hEventReset;
  HANDLE hEventVolumeUpdate;
  HANDLE hEventGetVolume;
  HANDLE hEventGetPadding;

};

