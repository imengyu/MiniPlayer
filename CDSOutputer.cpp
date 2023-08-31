#include "stdafx.h"
#include "CDSOutputer.h"
#include "CSoundPlayer.h"
#include "IPlayer.h"
#include <math.h>
#include "FastFourierTransform.h"
#include "DbgHlper.h"

IDirectSound8 *m_pDS = NULL;
bool dsLoaded = false;
DirectSoundCreate8Fun f_ds;
HMODULE hDs;
WCHAR testdatastr[64];

//pre=10^(db/2000)*100
//log10(x)=y;
//db=2000.0 * log10(pre/100);
//pre=db/2000

#define PRE_TO_DB(pre) 2000.0 * log10(pre/100.0)
#define DB_TO_PRE(db) pow(10, db / 2000.0)*100.0

CDSOutputer::CDSOutputer(CSoundPlayer * instance)
{
	parent = instance;
	hThreadEventReset = CreateEvent(NULL, TRUE, FALSE, L"ThreadEventReset");
	hThreadEventExit = CreateEvent(NULL, TRUE, TRUE, L"ThreadEventExit");
	hThreadEventEndOutPut = CreateEvent(NULL, TRUE, FALSE, L"ThreadEventEndOutPut");
	m_event[4] = hThreadEventReset;
	m_event[5] = hThreadEventEndOutPut;
	m_event[6] = hThreadEventExit;
}
CDSOutputer::~CDSOutputer()
{
	Destroy();
	if (playThread)TerminateThread(playThread, 0);
	playThread = NULL;
	CloseHandle(hThreadEventReset);
	CloseHandle(hThreadEventExit);
	CloseHandle(hThreadEventEndOutPut);
}

bool CDSOutputer::OnCopyData(CSoundPlayer * instance, LPVOID buf, DWORD buf_len)
{
	if (callback)
		return callback(parent,buf, buf_len);
	return false;
}
DWORD CDSOutputer::OnCheckEnd(CSoundPlayer* instance)
{
	if (checkEndCallback)
		return checkEndCallback(parent);
	return false;
}

bool CDSOutputer::Create(HWND hWnd, ULONG sample_rate, int channels,  int bits_per_sample)
{
	closed = false;
	last_data = false;
	next_data_end = false;
	if(hWnd==NULL) return parent->err(L"Init DirectSound failed : hWnd can not be nullptr!");
	this->sample_rate = sample_rate;
	this->channels = channels;
	this->bits_per_sample = bits_per_sample;
	bfs = BUFFERNOTIFYSIZE;
	bfs2 = static_cast<ULONG>(channels) * static_cast<ULONG>(bits_per_sample) / 8;
	if (!dsLoaded) {
		hDs = LoadLibrary(L"dsound.dll");
		if (!hDs)
			return parent->err(L"Init DirectSound failed : Load dsound.dll failed!");
		f_ds = (DirectSoundCreate8Fun)GetProcAddress(hDs, "DirectSoundCreate8");
		if (FAILED(f_ds(NULL, &m_pDS, NULL)))
			return parent->err(L"Init DirectSound failed : DirectSoundCreate8 failed!");
		if (FAILED(m_pDS->SetCooperativeLevel(hWnd, DSSCL_NORMAL)))
			return parent->err(L"Init DirectSound failed : SetCooperativeLevel");
		dsLoaded = true;
	}

	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2| DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = MAX_AUDIO_BUF * BUFFERNOTIFYSIZE;
	//WAVE Header  
	dsbd.lpwfxFormat = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
	dsbd.lpwfxFormat->wFormatTag = WAVE_FORMAT_PCM; /* format type */
	(dsbd.lpwfxFormat)->nChannels = static_cast<WORD>(channels); /* number of channels (i.e. mono, stereo...) */
	(dsbd.lpwfxFormat)->nSamplesPerSec = sample_rate; /* sample rate */
	(dsbd.lpwfxFormat)->nAvgBytesPerSec = sample_rate * static_cast<DWORD>((bits_per_sample / 8)*channels); /* for buffer estimation */
	(dsbd.lpwfxFormat)->nBlockAlign = static_cast<WORD>((bits_per_sample / 8)*channels); /* block size of data */
	(dsbd.lpwfxFormat)->wBitsPerSample = static_cast<WORD>(bits_per_sample); /* number of bits per sample of mono data */
	(dsbd.lpwfxFormat)->cbSize = 0;

	//Creates a sound buffer object to manage audio samples.   
	if (FAILED(m_pDS->CreateSoundBuffer(&dsbd, &m_pDSBuffer, NULL))) {
		return parent->err(L"DirectSound error : CreateSoundBuffer failed!");
	}

	if (FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8))) {
		return parent->err(L"DirectSound error.");
	}
	else if (m_setedVol) {
		SetVol(m_setedVol);
		m_setedVol = 0;
	}
	//Get IDirectSoundNotify8  
	if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify))) {
		return parent->err(L"DirectSound error.");
	}
	for (int i = 0; i<MAX_AUDIO_BUF; i++) {
		m_pDSPosNotify[i].dwOffset = i * BUFFERNOTIFYSIZE;
		m_event[i] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		m_pDSPosNotify[i].hEventNotify = m_event[i];
	}
	m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF, m_pDSPosNotify);
	m_pDSNotify->Release();

	offset = 0;
	cur_decorder_pos = 0;

	OutputDebugString(L"Created\n");

	return true;
}
bool CDSOutputer::Destroy()
{
	last_data = false;
	next_data_end = false;
	offset = 0;
	cur_decorder_pos = 0;
	EndOutPut();

	free(dsbd.lpwfxFormat);
	if (m_pDSBuffer8) {
		m_pDSBuffer8->Stop();
		m_pDSBuffer8->Release();
	}
	m_pDSBuffer8 = NULL;
	if (m_pDSBuffer) {
		m_pDSBuffer->Stop();
		m_pDSBuffer->Release();
	}
	m_pDSBuffer = NULL;
	closed = true;
	OutputDebugString(L"Destroyed\n");
	return false;
}

DWORD CDSOutputer::GetCurrPosSample()
{
	if (m_outputing)
		return cur_decorder_pos + GetOutPutingPosSample();
	return 0;
}
double CDSOutputer::GetCurrPos()
{
	if (m_outputing)
		return ((double)cur_decorder_pos / (double)sample_rate) + GetOutPutingPos();
	return 0.0;
}
DWORD CDSOutputer::GetOutPutingPosSample()
{
	if (m_outputing)
	{
		DWORD cp;
		if (SUCCEEDED(m_pDSBuffer8->GetCurrentPosition(&cp, NULL))) {
			return cp / bfs2;
		}
	}
	return 0;
}
DWORD CDSOutputer::GetBufferSizeSample() { 
	return bfs;
};
double CDSOutputer::GetOutPutingPos()
{
	if (m_outputing)
	{
		DWORD cp;
		if (SUCCEEDED(m_pDSBuffer8->GetCurrentPosition(&cp, NULL))) {
			return  (double)cp / (double)bfs;
		}
	}
	return 0.0;
}

bool CDSOutputer::IsOutPuting()
{
	return m_outputing;
}
bool CDSOutputer::StartOutPut()
{	
	if (!m_outputing)
	{
		if (playThread)
			WaitForSingleObject(m_event[6], INFINITE);
		ResetEvent(m_event[5]);
		ResetEvent(m_event[6]);
		m_outputing = true;
		playThread = (HANDLE)CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadFuncPCM, (void*)this, 0, &hThreadID);
		if (playThread == NULL)
			return parent->err(L"Failed create player thread.");
		cur_decorder_pos = 0;
		m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
		m_pDSBuffer8->SetCurrentPosition(0);
		return m_outputing;
	}
	return m_outputing;
}
bool CDSOutputer::EndOutPut()
{
	if (m_outputing) {
		SetEvent(m_event[5]);
		m_pDSBuffer8->Stop();
	}
	return true;
}
bool CDSOutputer::ResetOutPut()
{
	if (m_outputing)
	{
		//cur_decorder_pos = ((IPlayer*)parent)->UpdatePos();
		m_pDSBuffer->Stop();
		SetEvent(m_event[4]);
		return true;
	}
	else return StartOutPut();
}

int CDSOutputer::GetVol()
{
	LONG lg = 0;
	if (m_pDSBuffer8 != NULL) {
		m_pDSBuffer8->GetVolume(&lg);
	}
	return static_cast<int>(DB_TO_PRE(lg));
}
void CDSOutputer::SetVol(int vol)
{
	if (m_pDSBuffer8 != NULL) {
		if (vol != 0) m_pDSBuffer8->SetVolume((LONG)(PRE_TO_DB(vol)));
		else m_pDSBuffer8->SetVolume((LONG)(-10000));
	}
	else {
		m_setedVol = vol;
	}
}


int CDSOutputer::ThreadFuncPCM(void * lpdwParam)
{
	LPVOID buf = NULL;
	DWORD bytes = 0;
	bool copyDataRet = false;

	CDSOutputer*instance = static_cast<CDSOutputer*>(lpdwParam);
	OutputDebugString(L"ThreadStart\n");
	if (instance) {
		RESET:
		instance->offset = 0;
		instance->res = 0;

		while (instance->m_outputing) 
		{
			switch (instance->res)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				if (SUCCEEDED(instance->m_pDSBuffer8->Lock(instance->offset, BUFFERNOTIFYSIZE, &buf, &bytes, NULL, NULL, NULL)))
				{
					copyDataRet = instance->OnCopyData(instance->parent, buf, bytes);
					instance->m_pDSBuffer8->Unlock(buf, bytes, NULL, NULL);
					instance->offset += BUFFERNOTIFYSIZE;
					instance->offset %= BUFFERNOTIFYSIZE * MAX_AUDIO_BUF;
				}
				if (!copyDataRet) {
					auto leaveSamples = instance->OnCheckEnd(instance->parent);
					auto sleepMs = (DWORD)(leaveSamples / (double)instance->sample_rate * 1000);
					Sleep(sleepMs);
					((IPlayer*)instance->parent)->SetEndStatus();
					goto EXIT;
				}
				instance->m_outputing = copyDataRet;
			  break;
			case 4:
				ResetEvent(instance->m_event[4]);
				instance->m_pDSBuffer8->Stop();
				instance->m_pDSBuffer8->SetCurrentPosition(0);
				instance->m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
				goto RESET;
				break;
			case 5:				
				ResetEvent(instance->m_event[5]);
				goto EXIT;
				break;
			default:
				break;
			}

			instance->res = WaitForMultipleObjects(MAX_AUDIO_BUF + 2, instance->m_event, FALSE, INFINITE);
		}
	}
EXIT:
	instance->m_outputing = false;
	SetEvent(instance->m_event[5]);
	SetEvent(instance->m_event[6]);
	OutputDebugString(L"ExitThread\n");
	ExitThread(0);
	return 0;
}


