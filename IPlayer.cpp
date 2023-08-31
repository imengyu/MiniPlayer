#include "stdafx.h"
#include "IPlayer.h"
#include "CStringConv.h"
#include "mpg123.h"
#include <iostream>
#include <vector>
#include <time.h>
#include <MMSystem.h>

using namespace std;

#include "CPcmDecoder.h"
#include "CWavDecoder.h"
#include "CMP3Decoder.h"
#include "COggDecoder.h"
#include "CMidiDecoder.h"
#include "CWmaDecoder.h"
#include "CFLACDecoder.h"
#include "CAacDecoder.h"
#include "CAC3Decoder.h"

#include "CDSOutputer.h"
#include <Digitalv.h>

#define TIMER_FADE 26
#define TIMER_END_CHECK 27

std::vector<CDecoderPluginReal> decoders;

IPlayer::IPlayer(HWND hWnd)
{
	hostHWnd = hWnd;
	m_outputer = new CDSOutputer(this);
	m_outputer->SetOnCopyDataCallback(OnCopyData);
	m_outputer->SetOnCheckEndCallback(OnCheckEnd);
}
IPlayer::~IPlayer()
{
	if (m_playerStatus == Playing)
		Stop();

	Close();
	if(m_outputer)delete m_outputer;
}

bool IPlayer::Open(LPWSTR file)
{
	if (!m_LastDestroyed)
		Close();
	TStreamFormat thisFileFormat = GetFileFormat(file);
	if (thisFileFormat == TStreamFormat::sfUnknown) {
		err(L"UNKNOWN_FILE_FORMAT");
		return false;
	}
	if (m_openedFileFormat != thisFileFormat) {
		if (m_decoder)
		{
			if (m_decoder->IsOpened())
				m_decoder->Close();
			m_decoder->Release();
			m_decoder = NULL;
		}
	}
	if (m_decoder == NULL)
		m_decoder = CreateDecoderWithFormat(thisFileFormat);
	if (m_decoder != NULL) {
		m_openedFileFormat = thisFileFormat;
		if (m_decoder->Open(file)) {
			m_decoder->SeekToSec(0);
			m_playerStatus = TPlayerStatus::Opened;

			current_sample_rate = m_decoder->GetMusicSampleRate();
			current_channels = m_decoder->GetMusicChannelsCount();
			current_bits_per_sample = m_decoder->GetMusicBitsPerSample();
			if (current_sample_rate == -1 || current_channels == -1 || current_bits_per_sample == -1) {
				m_decoder->Close();
				return err(L"User canceled play.");
			}
			if (!m_outputer->Create(hostHWnd, current_sample_rate, current_channels, current_bits_per_sample))
				return false;

			if (playerVol != m_outputer->GetVol())m_outputer->SetVol(playerVol);

			time_t t;
			tm p;
			t = static_cast<long long>(m_decoder->GetMusicLength());
			gmtime_s(&p, &t);
			memset(isplayingAllTime, 0, sizeof(isplayingAllTime));
			wcsftime(isplayingAllTime, sizeof(isplayingAllTime) / sizeof(wchar_t), L"%M:%S", &p);
			m_LastDestroyed = FALSE;
			currentPlayPosSample = 0;

#if _DEBUG
			WCHAR teststr[100];
			swprintf_s(teststr, L"duration : %f sample : %d sample_rate: %d channel: %d bits_per_sample: %d\n",
				m_decoder->GetMusicLength(),
				m_decoder->GetMusicLengthSample(),
				m_decoder->GetMusicSampleRate(),
				m_decoder->GetMusicChannelsCount(),
				m_decoder->GetMusicBitsPerSample()
			);
			OutputDebugString(teststr);
#endif

			UpdatePos();

			return true;
		}
		else err(m_decoder->GetLastErr());
	}
	else err(L"Failed create decoder!\nBad file format!");
	return false;
}
bool IPlayer::Close()
{
	if (IsOpened()) {
		if (m_outputer != NULL) {
			m_outputer->Destroy();
		}
		if (m_decoder != NULL) {
			m_decoder->Close();
			delete m_decoder;
			m_decoder = NULL;
		}
		m_playerStatus = NotOpen;
		m_LastDestroyed = TRUE;
		return true;
	}
	else err(L"Not open.");
	return false;
}

bool IPlayer::PausePlay()
{
	if (m_playingMidi)
	{
		if (IsOpened())
		{
			DWORD music_playstyle = ((CMidiDecoder*)m_decoder)->GetStatus();
			switch (music_playstyle)
			{
			case 525:
				return Play();
			case 526:
				return Pause();
			default:
				return Restart();
			}
		}
	}
	else
	{
		if (m_playerStatus == Playing)
			return Pause();
		else if (m_playerStatus == Paused)
			return Play();
		else  if (m_playerStatus == Opened)
			return Play();
	}
	return false;
}
bool IPlayer::Play()
{
	if (IsOpened())
	{
		if (m_playingMidi)
		{
			if (m_playerStatus != TPlayerStatus::Playing)
			{
				MCI_PLAY_PARMS PlayParms;
				mciSendCommand(((CMidiDecoder*)m_decoder)->GetMidiId(), MCI_PLAY, 0,
					(DWORD)(LPVOID)&PlayParms);

				m_playerStatus = TPlayerStatus::Playing;
				return true;
			}
		}
		else {
			if (m_playerStatus != TPlayerStatus::Playing)
			{
				m_playerStatus = TPlayerStatus::Playing;
				if (!m_outputer->IsOutPuting())
					m_outputer->StartOutPut();
				return true;
			}
			else return true;
		}
	}
	return false;
}
bool IPlayer::Pause()
{
	if (IsOpened())
	{
		if (m_playingMidi)
		{
			if (m_playerStatus != TPlayerStatus::Paused)
			{
				MCI_PLAY_PARMS PlayParms;
				mciSendCommand(((CMidiDecoder*)m_decoder)->GetMidiId(), MCI_PAUSE, 0,
					(DWORD)(LPVOID)&PlayParms);
				m_playerStatus = TPlayerStatus::Paused;
				return true;
			}		
		}
		else {
			if (m_playerStatus != TPlayerStatus::Paused)
			{
				if (m_outputer->IsOutPuting())m_outputer->EndOutPut();
				m_playerStatus = TPlayerStatus::Paused;
				return true;
			}
			else return true;
		}
	}
	return false;
}
bool IPlayer::Stop()
{
	if (IsOpened())
	{
		if (m_playingMidi)
		{
			if (m_playerStatus != TPlayerStatus::Opened)
			{
				MCI_PLAY_PARMS PlayParms;
				mciSendCommand(((CMidiDecoder*)m_decoder)->GetMidiId(), MCI_STOP, 0,
					(DWORD)(LPVOID)&PlayParms);

				m_playerStatus = TPlayerStatus::Opened;
				return true;
			}
		}
		else {
			if (m_playerStatus != TPlayerStatus::Opened)
			{
				if (m_outputer->IsOutPuting())
					m_outputer->EndOutPut();

				m_playerStatus = TPlayerStatus::Opened;
				return true;
			}
			else return true;
		}
	}
	return false;
}
bool IPlayer::Restart()
{
	if (IsOpened())
	{
		if (m_playingMidi)
		{
			MCI_PLAY_PARMS PlayParms;
			PlayParms.dwFrom = 0;
			mciSendCommand(((CMidiDecoder*)m_decoder)->GetMidiId(), MCI_PLAY,
				MCI_FROM, (DWORD)(LPVOID)
				&PlayParms);
			m_playerStatus = TPlayerStatus::Playing;
		}
		else {
			m_decoder->SeekToSec(0);
			return Play();
		}
	}
	return false;
}

bool IPlayer::IsOpened()
{
	if (m_decoder)return m_decoder->IsOpened();
	return false;
}

CSoundDecoder * IPlayer::GetCurrDecoder()
{
	if (IsOpened())
		return m_decoder;
	return nullptr;
}
TStreamFormat IPlayer::GetFormat()
{
	if (IsOpened())
		return m_openedFileFormat;
	return TStreamFormat::sfUnknown;
}

IPlayer *currentFadePlayer = NULL;
bool currentFadeIn = false;
int currentFadeVal = 0;
double currentFadeOff = 5.0;
int endFadeVol = 0;

WCHAR teststr[128];

LPWSTR IPlayer::GetMusicPosTimeString() {
	return isplayingPosTime;
}
LPWSTR IPlayer::GetMusicLenTimeString() {
	return isplayingAllTime;
}
LPWSTR IPlayer::GetMusicTimeString() {
	return isplayingTime;
}
TPlayerStatus IPlayer::GetPlayerStatus()
{
	return m_playerStatus;
}
DWORD IPlayer::GetMusicPosSample()
{
	//精确时间为累计输出器输出的sample
	auto ouputerPos = m_outputer->GetCurrPosSample();

	if (ouputerPos != lastGetMusicPosSample && GetPlayerStatus() == Playing) {
		DWORD passedSamples = 0;
		if (ouputerPos > lastGetMusicPosSample)
			passedSamples = (ouputerPos - lastGetMusicPosSample);
		else
			passedSamples = (ouputerPos + (m_outputer->GetBufferSizeSample() - lastGetMusicPosSample));
		lastGetMusicPosSample = ouputerPos;
		currentPlayPosSample += passedSamples;
	}

	return currentPlayPosSample;
}
DWORD IPlayer::SetMusicPosSample(DWORD sample)
{
	if (IsOpened()) {
		currentPlayPosSample = sample;
		if (sample != m_decoder->GetCurSample()) {
			m_decoder->SeekToSample(sample);
			if (!m_playingMidi) m_outputer->ResetOutPut();
		}
	}
	return sample;
}
double IPlayer::GetMusicPos()
{
	if (m_decoder && m_decoder->IsOpened())
	{
		double sample = GetMusicPosSample();
		double c = sample / current_sample_rate;

		//生成时间
		time_t t;
		tm p;
		t = static_cast<long long>(c);
		gmtime_s(&p, &t);
		memset(isplayingTime, sizeof(isplayingTime), 0);
		wcsftime(isplayingTime, sizeof(isplayingAllTime) / sizeof(wchar_t), L"%M:%S", &p);
		wcscpy_s(isplayingPosTime, isplayingTime);
		wcscat_s(isplayingTime, L"/");
		wcscat_s(isplayingTime, isplayingAllTime);

#if _DEBUG
		swprintf_s(teststr, L"pos : %f sample : %f decoder CurSample : %d outputer pos : %d outputer CurSample : %d\n", 
			c,
			sample,
			m_decoder->GetCurSample(), 
			((CDSOutputer*)m_outputer)->cur_decorder_pos, 
			m_outputer->GetCurrPosSample()
		);
		OutputDebugString(teststr);
#endif

		return c;
	}
	return -1;
}
double IPlayer::GetMusicLength()
{
	if (m_decoder&&m_decoder->IsOpened())
		return m_decoder->GetMusicLength();
	else return -1;
}
DWORD  IPlayer::GetMusicLengthSample()
{
	if (m_decoder&&m_decoder->IsOpened())
		return m_decoder->GetMusicLengthSample();
	else return -1;
}
bool IPlayer::SetMusicPos(double sec)
{
	if (IsOpened()) {
		currentPlayPosSample = static_cast<DWORD>(sec * current_sample_rate);;
		if (sec != m_decoder->GetCurSec()) {
			m_decoder->SeekToSec(sec);
			if (!m_playingMidi)
				m_outputer->ResetOutPut();
		}
	}
	return false;
}
void IPlayer::FadeOut(int sec, int from, int to)
{
	currentFadeOff = (from - to)*0.1;
	currentFadePlayer = this;
	SetTimer(hostHWnd, TIMER_FADE, static_cast<UINT>(sec*0.1*1000), (TIMERPROC)OnFadeTime);
	currentFadeIn = false;
	currentFadeVal = from;
	endFadeVol = to;
}
void IPlayer::FadeIn(int sec, int from, int to)
{
	currentFadeOff = (to - from)*0.1;
	currentFadePlayer = this;
	SetTimer(hostHWnd, TIMER_FADE, static_cast<UINT>(sec*0.1 * 1000), (TIMERPROC)OnFadeTime);
	currentFadeIn = true;
	currentFadeVal = from;
	endFadeVol = to;
}
void IPlayer::SetDefOutPutSettings(ULONG sample_rate, int channels, int bits_per_sample)
{
	def_sample_rate = sample_rate;
	def_channels = channels;
	def_bits_per_sample = bits_per_sample;
	def_out_seted = true;
}
int IPlayer::GetPlayerVolume()
{
	if (m_playingMidi)
		return playerVol;
	return m_outputer->GetVol();
}
void IPlayer::SetPlayerVolume(int vol)
{
	playerVol = vol;
	if (m_playingMidi)
	{
		MCI_DGV_SETAUDIO_PARMS mciSetAudioPara;
		mciSetAudioPara.dwItem = MCI_DGV_SETAUDIO_VOLUME;
		mciSetAudioPara.dwValue = vol * 10;
		mciSendCommand(((CMidiDecoder*)m_decoder)->GetMidiId(), MCI_SETAUDIO, MCI_DGV_SETAUDIO_VALUE | MCI_DGV_SETAUDIO_ITEM, (DWORD)(LPVOID)&mciSetAudioPara);
	}
	else m_outputer->SetVol(vol);
}
bool IPlayer::IsPlayingMidi() {
	return m_playingMidi;
}

bool IPlayer::LoadDecoderPlugin(TStreamFormat format, LPWSTR path, LPCSTR createFun, LPCSTR destroyFun)
{
	std::vector<CDecoderPluginReal>::iterator it = find(decoders.begin(), decoders.end(), path);
	if (it != decoders.end())
		return err(L"Decoder have loaded.");
	else
	{
		if (format != sfUnknown)
		{
			if (_waccess(path, 0) == 0)
			{
				CDecoderPluginReal d;
				d.DllPath = path;
				d.TargetFormat = format;
				d.hModule = LoadLibrary(path);
				if(!d.hModule) return err(L"Decoder file not load.");
				d.CreateFun = (_CreateFun)GetProcAddress(d.hModule, createFun);
				d.DestroyFun = (_DestroyFun)GetProcAddress(d.hModule, destroyFun);
				if (d.CreateFun && d.DestroyFun)
				{
					decoders.push_back(d);
					return true;
				}
				else
				{
					FreeLibrary(d.hModule);
					return err(L"Decoder has not CreateFun and DestroyFun.");
				}
			}
			else return err(L"Decoder file not find.");
		}
		else return err(L"TStreamFormat set bad.");
	}
}
bool IPlayer::UnLoadDecoderPlugin(TStreamFormat format, LPWSTR path)
{
	std::vector<CDecoderPluginReal>::iterator it = find(decoders.begin(), decoders.end(), path);
	if (it != decoders.end())
	{
		auto d = *it;
		if (d.TargetFormat == format)
		{
			if (FreeLibrary(GetModuleHandle(d.DllPath)))
				return true;
			else 
				return err(L"FreeLibrary failed.");
		}
		else  return err(L"Bad Decoder format.");
	}
	else return err(L"Decoder not load.");
}
CSoundDecoder* IPlayer::CreateDecoderInPlugin(const TStreamFormat format)
{
	std::vector<CDecoderPluginReal>::iterator it = find(decoders.begin(), decoders.end(), format);
	if (it != decoders.end())
	{
		auto d = *it;
		CSoundDecoder*de = d.CreateFun(&d);
		de->CreateInPlugin = true;
		return de;
	}
	return nullptr;
}
bool IPlayer::DestroyDecoderInPlugin(const TStreamFormat format, CSoundDecoder*de)
{
	if (de->CreateInPlugin)
	{
		std::vector<CDecoderPluginReal>::iterator it = find(decoders.begin(), decoders.end(), format);
		if (it != decoders.end())
		{
			auto d = *it;
			d.DestroyFun(&d, de);
			decoders.erase(it);
		}
	}
	return false;
}

#define RETURN_TYPE(type) { fclose(f);return type; }

TStreamFormat IPlayer::GetFileFormat(const wchar_t * pchFileName)
{
	if (pchFileName == 0)
		return sfUnknown;

	// open file
	FILE* f;
	_wfopen_s(&f, pchFileName, L"rb");
	if (!f)
		return sfUnknown;

	char tmp[16];
	if (fread_s(tmp, sizeof(tmp), 1, 16, f) < 16) {
		fclose(f);
		return sfUnknown;
	}

	// check if file is wav
	// ====================================================================
	if (strncmp(tmp, "WAVE", 4) == 0)
		RETURN_TYPE(sfWav)
	if (strncmp(tmp, "WAVEfmt", 8) == 0)
		RETURN_TYPE(sfWav)
	if (strncmp(tmp, "RIFF", 4) == 0)
		RETURN_TYPE(sfWav)

	// check if file is midi
	// ====================================================================
	if (strncmp(tmp, "MThd", 4) == 0)
		RETURN_TYPE(sfMidi)

	// check if file is m4a
	// ====================================================================
	const char m4aHead[16] = {
		0x00, 0x00, 0x00, 0x20, 0x66, 0x74, 0x79, 0x70, 
		0x4D, 0x34, 0x41, 0x20, 0x00, 0x00, 0x00, 0x00
	};
	if (strncmp(tmp, m4aHead, sizeof(m4aHead)) == 0)
		RETURN_TYPE(sfM4a)

	// check if file is m4a
	// ====================================================================
	const char wmaHead[8] = {
		0x30, 0x26, (char)0xb2, (char)0x75, (char)0x8e, (char)0x66, (char)0xcf, (char)0x11
	};
	if (strncmp(tmp, wmaHead, sizeof(wmaHead)) == 0)
		RETURN_TYPE(sfWma)

	//	CHECK IG THIS IS FLAC FILE
	// ====================================================================
	if (strncmp(tmp, "fLaC", 4) == 0)
		RETURN_TYPE(sfFLAC)

	// check if file can be ogg, check first 4 bytes for OggS identification
	// ====================================================================
	if (strncmp(tmp, "OggS", 4) == 0)
	{
		fseek(f, 29, SEEK_SET);
		fread_s(tmp, sizeof(tmp), 1, 4, f);

		if (strncmp(tmp, "FLAC", 4) == 0)
			RETURN_TYPE(sfFLACOgg)
		else
			RETURN_TYPE(sfOgg)
	}


	fclose(f);

	// =================================================
	// parse filename and try to get file format using filename extension
	wchar_t* ext = wcsrchr((wchar_t*)pchFileName, L'.');
	if (ext)
	{
		ext++;

		if (_wcsicmp(ext, L"mp3") == 0)
			return sfMp3;
		if (_wcsicmp(ext, L"mp2") == 0)
			return sfMp3;
		if (_wcsicmp(ext, L"mp1") == 0)
			return sfMp3;
		if (_wcsicmp(ext, L"wma") == 0)
			return sfWma;
		if (_wcsicmp(ext, L"aac") == 0)
			return sfAacADTS;
		if (_wcsicmp(ext, L"ogg") == 0)
			return sfOgg;
		if (_wcsicmp(ext, L"wav") == 0)
			return sfWav;
		if (_wcsicmp(ext, L"flac") == 0)
			return sfFLAC;
		if (_wcsicmp(ext, L"oga") == 0)
			return sfFLACOgg;
		if (_wcsicmp(ext, L"ac3") == 0)
			return sfAC3;
		if (_wcsicmp(ext, L"pcm") == 0)
			return sfPCM;
		if (_wcsicmp(ext, L"midi") == 0)
			return sfMidi;
		if (_wcsicmp(ext, L"mid") == 0)
			return sfMidi;
		if (_wcsicmp(ext, L"m4a") == 0)
			return sfM4a;
		if (_wcsicmp(ext, L"mp4") == 0)
			return sfM4a;
	}

	return sfUnknown;
}

DWORD IPlayer::UpdatePos() {
	DWORD d = m_decoder->GetCurSample();
	return d;
}
DWORD IPlayer::CheckPlayEnd() {
	//检查真实播放进度是否到达末尾，是则停止
	return m_decoder->GetMusicLengthSample() - GetMusicPosSample();
}
void IPlayer::SetEndStatus() {
	m_playerStatus = PlayEnd;
	m_outputer->EndOutPut();
}
void IPlayer::StartEndCheck()
{
}

CSoundDecoder * IPlayer::CreateDecoderWithFormat(TStreamFormat f)
{
	switch (f)
	{
	case sfMp3:
		m_playingMidi = FALSE;
		return new CMP3Decoder();
	case sfOgg:
		m_playingMidi = FALSE;
		return new COggDecoder();
	case sfWav:
		m_playingMidi = FALSE;
		return new CWavDecoder();
	case sfPCM:
		m_playingMidi = FALSE;
		return new CPcmDecoder(this);
	case sfMidi:
		m_playingMidi = TRUE;
		return new CMidiDecoder(this);
	case sfWma:
		return new CWmaDecoder();
	case sfFLAC:
		return new CFLACDecoder(false);
	case sfFLACOgg:
		return new CFLACDecoder(true);
	case sfAacADTS:
		return new CAacDecoder();
	default:
		err(L"Unsupport file format.");
		break;
	}
	return nullptr;
}

bool IPlayer::err(wchar_t const* errmsg)
{
	wcscpy_s(lasterr, errmsg);
	return false;
}

DWORD IPlayer::OnCheckEnd(CSoundPlayer* instance) {
	return ((IPlayer*)instance)->CheckPlayEnd();
}
bool IPlayer::OnCopyData(CSoundPlayer*instance, LPVOID buf, DWORD buf_len)
{
	if (!instance->IsPlayingMidi())
	{
		auto read_size = ((IPlayer*)instance)->m_decoder->Read(buf, buf_len);
		if (read_size < buf_len) {
			memset((void*)((size_t)buf + read_size), 0, buf_len - read_size);
			return false;
		}
		return true;
	}	
	return false;
}

VOID CALLBACK IPlayer::OnFadeTime(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
	if (!currentFadePlayer) { 
		KillTimer(hwnd, TIMER_FADE); 
		return; 
	}
	if (currentFadeIn)
	{
		if (currentFadeVal < endFadeVol)
		{
			currentFadeVal += static_cast<int>(currentFadeOff);
			currentFadePlayer->SetPlayerVolume(currentFadeVal);
		}
		else
		{
			currentFadeVal = endFadeVol;
			currentFadePlayer->SetPlayerVolume(endFadeVol);
			KillTimer(hwnd, TIMER_FADE);
		}
	}
	else
	{
		if (currentFadeVal > endFadeVol)
		{
			currentFadeVal -= static_cast<int>(currentFadeOff);
			currentFadePlayer->SetPlayerVolume(currentFadeVal);
		}
		else
		{
			currentFadeVal = endFadeVol;
			currentFadePlayer->SetPlayerVolume(endFadeVol);
			KillTimer(hwnd, TIMER_FADE);
		}
	}
}

#pragma region FFT Draw

void IPlayer::DrawFFTOnHDCSmall(HDC hdc)
{
	m_outputer->DrawFFTOnHDCSmall(hdc);
}

void IPlayer::SetFFTHDC(HDC hdc)
{
	ffthdc = hdc;
	m_outputer->SetFFTHDC(hdc);
}
void IPlayer::DrawFFTOnHDC(HDC hdc)
{
	m_outputer->DrawFFTOnHDC(hdc);
}

#pragma endregion

#pragma region CDecoderPlugin

bool CDecoderPluginReal::operator==(const CDecoderPluginReal left)
{
	return wcscmp(DllPath, left.DllPath) == 0;
}
bool CDecoderPluginReal::operator!=(const CDecoderPluginReal left)
{
	return wcscmp(DllPath, left.DllPath) != 0;
}
bool CDecoderPluginReal::operator==(const CDecoderPluginReal& left)
{
	return wcscmp(DllPath, left.DllPath) == 0;
}
bool CDecoderPluginReal::operator!=(const CDecoderPluginReal& left)
{
	return wcscmp(DllPath, left.DllPath) != 0;
}
bool CDecoderPluginReal::operator==(const TStreamFormat right)
{
	return TargetFormat == right;
}
bool CDecoderPluginReal::operator!=(const TStreamFormat right)
{
	return TargetFormat != right;
}
bool CDecoderPluginReal::operator==(const LPWSTR right)
{
	return wcscmp(DllPath, right) == 0;
}
bool CDecoderPluginReal::operator!=(const LPWSTR right)
{
	return wcscmp(DllPath, right) != 0;
}

#pragma endregion

#pragma region Fast audio duration Reader

double IPlayer::GetAudioDurationFast(LPWSTR file)
{
	auto type = IPlayer::GetFileFormat(file);
	switch (type)
	{
	case sfMp3: {
		int ret = -1;
		auto _handle = mpg123_new(NULL, &ret);
		if (_handle == NULL || ret != MPG123_OK)
			return false;

		LPCSTR str = CStringConv::w2a(file);
		if (mpg123_open(_handle, str) != MPG123_OK) {
			CStringConv::del(str);
			mpg123_delete(_handle);
			return 0;
		}
		CStringConv::del(str);

		long rate = 0;
		int channel = 0, encoding = 0;
		if (mpg123_getformat(_handle, &rate, &channel, &encoding) != MPG123_OK) {
			mpg123_delete(_handle);
			return 0;
		}
		auto len = mpg123_length(_handle);
		mpg123_delete(_handle);
		return len / (double)rate;
	}
	case sfOgg: {
		FILE* _Stream;
		_wfopen_s(&_Stream, file, L"rb");
		if (_Stream)
		{
			OggVorbis_File _OggFile;
			if (ov_open(_Stream, &_OggFile, NULL, 0) < 0)
				return 0;
			if (ov_streams(&_OggFile) != 1)
				goto FAIL_CLEAN_ALL;

			vorbis_info* pInfo = NULL;
			pInfo = ov_info(&_OggFile, -1);
			if (pInfo == NULL)
				goto FAIL_CLEAN_ALL;

			auto m_SampleRate = static_cast<ULONG>(pInfo->rate);
			auto m_Samples = ov_pcm_total(&_OggFile, -1);
			return static_cast<double>(m_Samples) / (double)m_SampleRate;

		FAIL_CLEAN_ALL:
			ov_clear(&_OggFile);
			fclose(_Stream);
			return 0;
		}
	}
	case sfWav: {
		FILE* pFile;
		WAV_HEADER pWavHead = { 0 };
		_wfopen_s(&pFile, file, L"rb");
		if (pFile) {
			fread_s(&pWavHead, sizeof(WAV_HEADER), sizeof(pWavHead), 1, pFile);
			fseek(pFile, 0, SEEK_END);

			auto length = ftell(pFile);
			auto duration = (double)length / (double)(pWavHead.numChannels * 2) / (double)pWavHead.sampleRate;

			fclose(pFile);
			return duration;
		}
	}
	}
	return 0;
}

#pragma endregion