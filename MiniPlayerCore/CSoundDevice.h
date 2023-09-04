#pragma once
#include <Audioclient.h>
#include <mmdeviceapi.h>
#define BUFFERNOTIFYSIZE 192000

class CSoundPlayer;

typedef bool(*OnCopyDataCallback)(CSoundPlayer* instance, LPVOID buf, DWORD  buf_len);

class CSoundDevice
{
public:
  CSoundDevice(CSoundPlayer* parent);
  ~CSoundDevice();

  bool Create();
  void Destroy();
  void Reset();

  void SetOnCopyDataCallback(OnCopyDataCallback callback);

private:
  CSoundPlayer* parent;
  bool createSuccess = false;

  OnCopyDataCallback copyDataCallback;

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

};

