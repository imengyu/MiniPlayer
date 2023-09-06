#include "pch.h"
#include "CSoundDevice.h"
#include "CSoundPlayer.h"
#include "StringHelper.h"
#include "DbgHelper.h"
#include <thread>

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);

CSoundDevice::CSoundDevice(CSoundPlayerImpl* parent)
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
    SetEvent(hEventDestroy);
    ResetEvent(hEventDestroyDone);
    WaitForSingleObject(hEventDestroyDone, INFINITE);
    ResetEvent(hEventDestroyDone);
  }
}
void CSoundDevice::Reset()
{
  if (!createSuccess)
    return;
  SetEvent(hEventReset);
  ResetEvent(hEventResetDone);
  WaitForSingleObject(hEventResetDone, INFINITE);
  ResetEvent(hEventResetDone);
}
void CSoundDevice::Stop()
{
  if (!createSuccess)
    return;
  SetEvent(hEventStop);
}
void CSoundDevice::Start()
{
  if (!createSuccess)
    return;
  SetEvent(hEventPlay);
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

  //�̵߳ȴ������Ϣ��������Ϣִ�в�ͬ������
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

  //hr = pAudioClient->GetMixFormat(&pwfx);
  //EXIT_ON_ERROR(hr);

  //���ص�ǰ��Ƶ��ʽ
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

  //��ʼ��
  hr = pAudioClient->Initialize(
    AUDCLNT_SHAREMODE_SHARED,
    AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
    0,
    0,
    pwfx,
    NULL
  );
  EXIT_ON_ERROR(hr);

  hr = pAudioClient->SetEventHandle(device->hEventLoadData);
  EXIT_ON_ERROR(hr);

  // ��ȡ�ѷ��仺������ʵ�ʴ�С��
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

  // ץȡ�������������г�ʼ��������
  hr = pRenderClient->GetBuffer(device->bufferFrameCount, &pData);
  EXIT_ON_ERROR(hr);

  // ����ʼ���ݼ��ص������������С�
  device->copyDataCallback(device->parent, pData, device->bufferFrameCount * pwfx->nBlockAlign);

  hr = pRenderClient->ReleaseBuffer(device->bufferFrameCount, flags);
  EXIT_ON_ERROR(hr);

  // �������Ļ�������ʵ�ʳ���ʱ�䡣
  hnsActualDuration = (REFERENCE_TIME)((double)REFTIMES_PER_SEC * device->bufferFrameCount / pwfx->nSamplesPerSec);

  //ѭ��
  while (device->createSuccess)
  {
    switch (waitMessagesResult)
    {
    case 0:
      //�˳��߳�
      ResetEvent(device->hEventDestroy);
      goto EXIT;
    case 1:
      //��ʼ����
      ResetEvent(device->hEventPlay);
      pAudioClient->Start();
      device->isStarted = true;
      break;
    case 2:
      //ֹͣ����
      ResetEvent(device->hEventStop);
      pAudioClient->Stop();
      device->isStarted = false;
      break;
    case 3:
      //��������
      ResetEvent(device->hEventLoadData);

      hr = pAudioClient->GetCurrentPadding(&device->numFramesPadding);
      EXIT_ON_ERROR(hr)

      numFramesAvailable = device->bufferFrameCount - device->numFramesPadding;
      if (numFramesAvailable == 0)
        break;

      hr = pRenderClient->GetBuffer(numFramesAvailable, &pData);
      EXIT_ON_ERROR(hr)
        
      //��ȡ
      hasMoreData = device->copyDataCallback(device->parent, pData, numFramesAvailable * pwfx->nBlockAlign);

      hr = pRenderClient->ReleaseBuffer(numFramesAvailable, flags);
      EXIT_ON_ERROR(hr)

      if (!hasMoreData) {
        // �ȴ��������е����һ�����ݲ��ź���ֹͣ��
        Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2));

        //ֹͣ�����˳�
        pAudioClient->Stop();
        goto EXIT;
      }
      break;
    case 4:
      //���ò���
      ResetEvent(device->hEventReset);

      pAudioClient->Stop();
      pAudioClient->Reset();
      goto RESET;
    case 5:
      //��������
      ResetEvent(device->hEventVolumeUpdate);

      hr = pAudioStreamVolume->GetChannelCount(&channelCount);
      if (SUCCEEDED(hr)) 
        pAudioStreamVolume->SetAllVolumes(channelCount, device->currentVolume);
      break;
    case 6:
      //������ȡ
      ResetEvent(device->hEventGetVolume);

      hr = pAudioStreamVolume->GetChannelCount(&channelCount);
      if (SUCCEEDED(hr)) {
        pAudioStreamVolume->GetAllVolumes(channelCount, device->currentVolume);
      }
      break;
    case 7:
      //λ�û�ȡ
      ResetEvent(device->hEventGetPadding);
      pAudioClient->GetCurrentPadding(&device->numFramesPadding);
      break;
    default:
      break;
    }

    //�ȴ���Ϣ
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

  device->isStarted = false;
  device->createSuccess = false;
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