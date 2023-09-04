#include "pch.h"
#include "CSoundDevice.h"
#include "CSoundPlayer.h"
#include "StringHelper.h"
#include <thread>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

CSoundDevice::CSoundDevice(CSoundPlayer* parent)
{
  this->parent = parent;
}
CSoundDevice::~CSoundDevice()
{
  if (createSuccess)
    Destroy();
}

void CSoundDevice::SetOnCopyDataCallback(OnCopyDataCallback callback)
{
  copyDataCallback = callback;
}

bool CSoundDevice::Create()
{
  if (createSuccess)
    return true;

  std::thread playerThread(PlayerThread, this);
  playerThread.detach();

  WaitForSingleObject(hEventCreateDone, 100);
  ResetEvent(hEventCreateDone);

  return createSuccess;
}
void CSoundDevice::Destroy()
{
  SetEvent(hEventDestroy);
  WaitForSingleObject(hEventDestroyDone, INFINITE);
  ResetEvent(hEventDestroyDone);
}
void CSoundDevice::Reset()
{
  SetEvent(hEventReset);
  WaitForSingleObject(hEventResetDone, INFINITE);
  ResetEvent(hEventResetDone);
}

//100ns
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define EXIT_ON_ERROR(hresut) if (FAILED(hresut)) { device->parent->SetLastError(PLAYER_ERROR_OUTPUT_ERROR, \
  StringHelper::FormatString(L"CSoundDevice create failed in line %d with hresult: %0x08X", __LINE__, hr).c_str()); \
  SetEvent(device->hEventCreateDone); \
  device->createSuccess = false; \
  goto EXIT; }

void CSoundDevice::PlayerThread(void* p)
{
  auto device = (CSoundDevice*)p;
  HRESULT hr;
  REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
  REFERENCE_TIME hnsActualDuration;
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDevice* pDevice = NULL;
  IAudioClient* pAudioClient = NULL;
  IAudioRenderClient* pRenderClient = NULL;
  UINT32 bufferFrameCount;
  DWORD flags = 0;
  BYTE* pData;
  WAVEFORMATEX* pwfx;

  UINT32 numFramesAvailable;
  UINT32 numFramesPadding;
  bool hasMoreData;


  //线程等待多个消息，根据消息执行不同的任务
  int waitMessagesResult = 0;
  HANDLE waitMessages[6] = {
     device->hEventDestroy,
     device->hEventPlay,
     device->hEventStop,
     device->hEventLoadData,
     device->hEventReset,
  };

  hr = CoInitialize(0);
  EXIT_ON_ERROR(hr);

  pwfx->nSamplesPerSec = device->parent->GetSampleRate();
  pwfx->nChannels = device->parent->GetChannelsCount();
  pwfx->wFormatTag = WAVE_FORMAT_PCM;
  pwfx->wBitsPerSample = device->parent->GetBitPerSample();
  pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
  pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;

  hr = CoCreateInstance(
    CLSID_MMDeviceEnumerator, NULL,
    CLSCTX_ALL, IID_IMMDeviceEnumerator,
    (void**)&pEnumerator);
  EXIT_ON_ERROR(hr);

  hr = pEnumerator->GetDefaultAudioEndpoint(
    eRender, eConsole, &pDevice);
  EXIT_ON_ERROR(hr);

  hr = pDevice->Activate(
    IID_IAudioClient, CLSCTX_ALL,
    NULL, (void**)&pAudioClient);
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->GetMixFormat(&pwfx);
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->Initialize(
    AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
    hnsRequestedDuration,
    0,
    pwfx,
    NULL);
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->SetEventHandle(device->hEventLoadData);
  EXIT_ON_ERROR(hr);

  // 获取已分配缓冲区的实际大小。
  hr = pAudioClient->GetBufferSize(&bufferFrameCount);
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->GetService(
    IID_IAudioRenderClient,
    (void**)&pRenderClient);
  EXIT_ON_ERROR(hr);

RESET:
  hasMoreData = false;
  SetEvent(device->hEventResetDone);

  // 抓取整个缓冲区进行初始填充操作。
  hr = pRenderClient->GetBuffer(bufferFrameCount, &pData);
  EXIT_ON_ERROR(hr);

  // 将初始数据加载到共享缓冲区中。
  device->copyDataCallback(device->parent, pData, bufferFrameCount);

  hr = pRenderClient->ReleaseBuffer(bufferFrameCount, flags);
  EXIT_ON_ERROR(hr);

  // 计算分配的缓冲区的实际持续时间。
  hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;
  
  //循环
  while (device->createSuccess)
  {
    switch (waitMessagesResult)
    {
    case 0:
      //退出线程
      ResetEvent(device->hEventDestroy);
      goto EXIT;
    case 1:
      //开始播放
      ResetEvent(device->hEventPlay);
      pAudioClient->Start();
      break;
    case 2:
      //停止播放
      ResetEvent(device->hEventStop);
      pAudioClient->Stop();
      break;
    case 3:
      //加载数据

      hr = pAudioClient->GetCurrentPadding(&numFramesPadding);
      EXIT_ON_ERROR(hr)

        numFramesAvailable = bufferFrameCount - numFramesPadding;

      hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
      EXIT_ON_ERROR(hr)

        //读取
        hasMoreData = device->copyDataCallback(device->parent, pData, numFramesAvailable);

      hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
      EXIT_ON_ERROR(hr)

      if (!hasMoreData) {
        // 等待缓冲区中的最后一个数据播放后再停止。
        Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

        //停止并且退出
        pAudioClient->Stop();
        goto EXIT;
      }
      break;
    case 4:
      //重置播放
      ResetEvent(device->hEventReset);

      pAudioClient->Stop();
      pAudioClient->Reset();
      goto RESET;
    default:
      break;
    }


    //等待消息
    waitMessagesResult = WaitForMultipleObjects(
      sizeof(waitMessages) / sizeof(HANDLE),
      waitMessages,
      FALSE,
      INFINITE
    );
  }

EXIT:
  if (pwfx)
    CoTaskMemFree(pwfx);
  device->createSuccess = false;
  if (pEnumerator)
    pEnumerator->Release();
  if (pDevice)
    pDevice->Release();
  if (pAudioClient)
    pAudioClient->Release();
  if (pRenderClient)
    pRenderClient->Release();

  CoUninitialize();
}
