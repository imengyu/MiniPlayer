#pragma once
#include "CSoundOutPuter.h"
#include "CSoundFFT.h"
#include <stdio.h>  
#include <stdlib.h>  
#include <Windows.h>
#include <MMSystem.h>

#include <dsound.h>  

#define WAVE_FORMAT_PCM 1
#define MAX_AUDIO_BUF 4   
#define BUFFERNOTIFYSIZE 192000   
#define ROW_LOCAL_COUNT 32

#define DELAY 1
#define XINTERVAL 1
#define YINTERVAL 1
#define XINTERVALSMALL 1
#define YINTERVALSMALL 1

typedef HRESULT (WINAPI *DirectSoundCreate8Fun)(__in_opt LPCGUID pcGuidDevice, __deref_out LPDIRECTSOUND8 *ppDS8, __null LPUNKNOWN pUnkOuter);

class CDSOutputer :
	public CSoundOutPuter
{
public:
	CDSOutputer(CSoundPlayer * instance);
	~CDSOutputer();

	bool OnCopyData(CSoundPlayer *instance, LPVOID buf, DWORD  buf_len) override;

	bool Create(HWND hWnd, ULONG sample_rate = 44100,  //PCM sample rate  
		int channels = 2,       //PCM channel number  
		int bits_per_sample = 16) override;
	bool Destroy() override;

	ULONG sample_rate = 44100;  //PCM sample rate  
	int channels = 2;         //PCM channel number  
	int bits_per_sample = 16; //bits per sample  

	DWORD GetCurrPosSample()override;
	double GetCurrPos()override;
	DWORD GetOutPutingPosSample()override;
	double GetOutPutingPos()override;
	bool IsOutPuting() override;
	bool StartOutPut() override;
	bool EndOutPut() override;
	bool ResetOutPut() override;

	int GetVol() override;
	void SetVol(int vol) override;

	DWORD cur_decorder_pos;
private:
	bool last_data = false;
	bool next_data_end = false;
	bool closed = false;

	DWORD bfs, bfs2;
	CSoundPlayer * parent;
	IDirectSoundBuffer8 *m_pDSBuffer8 = NULL; //used to manage sound buffers.  
	IDirectSoundBuffer *m_pDSBuffer = NULL;
	IDirectSoundNotify8 *m_pDSNotify = NULL;
	DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
	HANDLE m_event[MAX_AUDIO_BUF + 2];

	DSBUFFERDESC dsbd;
	HANDLE playThread;
	DWORD hThreadID;

	static int WINAPI ThreadFuncPCM(void* lpdwParam);
	DWORD offset = 0;
	DWORD res = WAIT_OBJECT_0;
	HANDLE hThreadEventReset;
	HANDLE hThreadEventExit;
	HANDLE hThreadEventEndOutPut;
	int m_setedVol = 0;
	bool m_outputing = false;

	bool Signaled = true;
};

