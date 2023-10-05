#include "pch.h"
#include "CSoundDevice.h"
#include "CSoundPlayer.h"
#include "StringHelper.h"
#include "DbgHelper.h"
#include "Logger.h"
#include <thread>

#define LOG_TAG "CSoundDevice"

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);

CSoundDevice::CSoundDevice(CSoundDeviceHoster* parent)
{
  this->parent = parent;
  memset(currentVolume, 0, sizeof(currentVolume));
  hEventCreateDone = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventDestroyDone = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventResetDone = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventDestroy = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventPlay = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventStop = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventLoadData = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventReset = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventVolumeUpdate = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventGetVolume = CreateEvent(NULL, TRUE, FALSE, NULL);
  hEventGetPadding = CreateEvent(NULL, TRUE, FALSE, NULL);
}
CSoundDevice::~CSoundDevice()
{
  if (createSuccess)
    Destroy();
  CloseHandle(hEventCreateDone);
  CloseHandle(hEventDestroyDone);
  CloseHandle(hEventResetDone);
  CloseHandle(hEventDestroy);
  CloseHandle(hEventPlay);
  CloseHandle(hEventStop);
  CloseHandle(hEventLoadData);
  CloseHandle(hEventReset);
  CloseHandle(hEventVolumeUpdate);
  CloseHandle(hEventGetVolume);
  CloseHandle(hEventGetPadding);
}

void CSoundDevice::SetOnCopyDataCallback(OnCopyDataCallback callback)
{
  copyDataCallback = callback;
}

float CSoundDevice::GetVolume(int index)
{
  SetEvent(hEventGetVolume);
  WaitForSingleObject(hEventGetVolume, INFINITE);
  return currentVolume[index];
}
void CSoundDevice::SetVolume(int index, float value)
{
  if (index == -1) {
    for (size_t i = 0; i < sizeof(currentVolume) / sizeof(float); i++)
      currentVolume[i] = value;
  }
  else
    currentVolume[index] = value;
  SetEvent(hEventVolumeUpdate);
}

UINT32 CSoundDevice::GetPosition()
{
  if (!createSuccess)
    return 0;
  SetEvent(hEventGetPadding);
  WaitForSingleObject(hEventGetPadding, INFINITE);
  ResetEvent(hEventGetPadding);
  return numFramesPadding;
}

CSoundDeviceDeviceDefaultFormatInfo& CSoundDevice::RequestDeviceDefaultFormatInfo()
{
  if (deviceDefaultFormatInfo.sampleRate == 0) {
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioRenderClient* pRenderClient = NULL;
    WAVEFORMATEX* pwfx = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    if (FAILED(hr)) {
      LOGEF(LOG_TAG, "CoCreateInstance failed in line %d with HRESULT: 0x%08X", hr); 
      goto EXIT;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr)) {
      LOGEF(LOG_TAG, "pEnumerator->GetDefaultAudioEndpoint failed in line %d with HRESULT: 0x%08X", hr); 
      goto EXIT;
    }

    hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    if (FAILED(hr)) {
      LOGEF(LOG_TAG, "pDevice->Activate failed in line %d with HRESULT: 0x%08X", hr);
      goto EXIT;
    }

    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr)) {
      LOGEF(LOG_TAG, "pAudioClient->GetMixFormat failed in line %d with HRESULT: 0x%08X", hr);
      goto EXIT;
    }

    deviceDefaultFormatInfo.sampleRate = pwfx->nSamplesPerSec;
    deviceDefaultFormatInfo.channels = pwfx->nChannels;
    switch (pwfx->wBitsPerSample)
    {
    case 8: deviceDefaultFormatInfo.fmt = AVSampleFormat::AV_SAMPLE_FMT_U8; break;
    case 16: deviceDefaultFormatInfo.fmt = AVSampleFormat::AV_SAMPLE_FMT_S16; break;
    case 32: deviceDefaultFormatInfo.fmt = AVSampleFormat::AV_SAMPLE_FMT_S32; break;
    }

  EXIT:
    if (pEnumerator)
      pEnumerator->Release();
    if (pDevice)
      pDevice->Release();
    if (pAudioClient)
      pAudioClient->Release();
    if (pRenderClient)
      pRenderClient->Release();
  }

  return deviceDefaultFormatInfo;
}

bool CSoundDevice::Create()
{
  if (createSuccess)
    return true;

  std::thread playerThread(PlayerThread, this);
  playerThread.detach();

  ResetEvent(hEventCreateDone);
  WaitForSingleObject(hEventCreateDone, INFINITE);
  ResetEvent(hEventCreateDone);

  return createSuccess;
}
void CSoundDevice::Destroy()
{
  if (createSuccess) {
    ResetEvent(hEventDestroyDone);
    SetEvent(hEventDestroy);
    WaitForSingleObject(hEventDestroyDone, 100);
  }
}
void CSoundDevice::Reset()
{
  if (!createSuccess)
    return;
  SetEvent(hEventReset);
  ResetEvent(hEventResetDone);
  WaitForSingleObject(hEventResetDone, 100);
  ResetEvent(hEventResetDone);
}
void CSoundDevice::Stop()
{
  if (!createSuccess)
    return;
  SetEvent(hEventStop);
}
bool CSoundDevice::Start()
{
  if (!createSuccess && !Create())
    return false;
  SetEvent(hEventPlay);
  return true;
}

//100ns
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define EXIT_ON_ERROR(hresut) if (FAILED(hresut)) { device->parent->SetLastError(PLAYER_ERROR_OUTPUT_ERROR, \
  StringHelper::FormatString(L"CSoundDevice error in line %d with HRESULT: 0x%08X", __LINE__, hr).c_str()); \
  SetEvent(device->hEventCreateDone); \
  device->createSuccess = false; \
   hasError = true; \
  goto EXIT; }

void CSoundDevice::PlayerThread(void* p)
{
  auto device = (CSoundDevice*)p;
  HRESULT hr;
  REFERENCE_TIME hnsActualDuration;
  IMMDeviceEnumerator* pEnumerator = NULL;
  IMMDevice* pDevice = NULL;
  IAudioClient* pAudioClient = NULL;
  IAudioRenderClient* pRenderClient = NULL;
  IAudioStreamVolume* pAudioStreamVolume = NULL;
  DWORD flags = 0;
  BYTE* pData;
  WAVEFORMATEX* pwfx = nullptr;

  UINT32 channelCount;
  UINT32 numFramesAvailable;
  bool hasMoreData;
  bool hasError = false;

  //线程等待多个消息，根据消息执行不同的任务
  int waitMessagesResult = -1;
  HANDLE waitMessages[8] = {
     device->hEventDestroy,
     device->hEventPlay,
     device->hEventStop,
     device->hEventLoadData,
     device->hEventReset,
     device->hEventVolumeUpdate,
     device->hEventGetVolume,
     device->hEventGetPadding
  };

  hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
  EXIT_ON_ERROR(hr);

  hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
  EXIT_ON_ERROR(hr);

  hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
  EXIT_ON_ERROR(hr);

  hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
  EXIT_ON_ERROR(hr);

  device->shouldReSample = device->parent->GetShouldReSample();
  if (device->shouldReSample) {

    //加载当前音频格式
    pwfx = (WAVEFORMATEX*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
    if (!pwfx)
      goto EXIT;

    pwfx->nSamplesPerSec = device->parent->GetSampleRate();
    pwfx->nChannels = device->parent->GetChannelsCount();
    pwfx->wFormatTag = WAVE_FORMAT_PCM;
    pwfx->wBitsPerSample = device->parent->GetBitPerSample();
    pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
    pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
    pwfx->cbSize = 0;
  }
  else {
    hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr);
  }

  //初始化
  hr = pAudioClient->Initialize(
    AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_STREAMFLAGS_EVENTCALLBACK | 
      (device->shouldReSample ? (AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY) : 0),
    0,
    0,
    pwfx,
    NULL
  );
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->SetEventHandle(device->hEventLoadData);
  EXIT_ON_ERROR(hr);

  // 获取已分配缓冲区的实际大小。
  hr = pAudioClient->GetBufferSize(&device->bufferFrameCount);
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pRenderClient);
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->GetService(IID_IAudioStreamVolume, (void**)&pAudioStreamVolume);
  EXIT_ON_ERROR(hr);

RESET:
  hasMoreData = false;
  device->isStarted = false;
  device->createSuccess = true;

  SetEvent(device->hEventResetDone);
  SetEvent(device->hEventCreateDone);

  // 抓取整个缓冲区进行初始填充操作。
  hr = pRenderClient->GetBuffer(device->bufferFrameCount, &pData);
  EXIT_ON_ERROR(hr);

  // 将初始数据加载到共享缓冲区中。
  device->copyDataCallback(device->parent, pData, device->bufferFrameCount * pwfx->nBlockAlign);

  hr = pRenderClient->ReleaseBuffer(device->bufferFrameCount, flags);
  EXIT_ON_ERROR(hr);

  // 计算分配的缓冲区的实际持续时间。
  hnsActualDuration = (REFERENCE_TIME)((double)REFTIMES_PER_SEC * device->bufferFrameCount / pwfx->nSamplesPerSec);

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
      device->isStarted = true;
      break;
    case 2:
      //停止播放
      ResetEvent(device->hEventStop);
      pAudioClient->Stop();
      device->isStarted = false;
      break;
    case 3:
      //加载数据
      ResetEvent(device->hEventLoadData);

      hr = pAudioClient->GetCurrentPadding(&device->numFramesPadding);
      EXIT_ON_ERROR(hr)

      numFramesAvailable = device->bufferFrameCount - device->numFramesPadding;
      if (numFramesAvailable == 0)
        break;

      hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
      EXIT_ON_ERROR(hr)
        
      //读取
      hasMoreData = device->copyDataCallback(device->parent, pData, numFramesAvailable * pwfx->nBlockAlign);

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
    case 5:
      //音量更改
      ResetEvent(device->hEventVolumeUpdate);

      hr = pAudioStreamVolume->GetChannelCount(&channelCount);
      if (SUCCEEDED(hr)) 
        pAudioStreamVolume->SetAllVolumes(channelCount, device->currentVolume);
      break;
    case 6:
      //音量获取
      ResetEvent(device->hEventGetVolume);

      hr = pAudioStreamVolume->GetChannelCount(&channelCount);
      if (SUCCEEDED(hr)) {
        pAudioStreamVolume->GetAllVolumes(channelCount, device->currentVolume);
      }
      break;
    case 7:
      //位置获取
      ResetEvent(device->hEventGetPadding);
      pAudioClient->GetCurrentPadding(&device->numFramesPadding);
      break;
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
  if (device->shouldReSample && pwfx)
    CoTaskMemFree(pwfx);

  device->isStarted = false;
  device->createSuccess = false;

  if (device->parent)
    device->parent->NotifyPlayEnd(hasError);

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
