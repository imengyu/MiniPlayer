#pragma once
#include "CSoundDecoder.h"
#include "CSoundDevice.h"
#include "CSoundPlayerExport.h"
#include <string>

//²¥·ÅÆ÷ÊµÀý
class CSoundPlayer : public CSoundPlayerBase
{
public:
	bool Load(const wchar_t* path);
	bool Close();
	bool Play();
	bool Pause();
	bool Stop();
	bool Restart();

	double GetPosition();
	void SetPosition(double second);
	double GetDuration();

	TPlayerStatus GetState();
	TStreamFormat GetFormat();

	void SetVolume(int volume);
	int GetVolume();

	void SetLastError(int code, const wchar_t* errmsg);
	void SetLastError(int code, const char* errmsg);
	int GetLastError() { return lastErrorCode; }
	const wchar_t* GetLastErrorMessage();

	unsigned long GetSampleRate() { return currentSampleRate; }
	int GetBitPerSample() { return currentBitsPerSample; }
	int GetChannelsCount() { return currentChannels; }


	unsigned int GetDurationSample();
	unsigned int GetPositionSample();
	void SetPositionSample(unsigned int sample);

private:
	CSoundDecoder* CreateDecoderWithFormat(TStreamFormat f);

private:
	CSoundDecoder* decoder = nullptr;

	TStreamFormat openedFileFormat = TStreamFormat::sfUnknown;

	ULONG currentSampleRate = 0;
	int currentChannels = 0;
	int currentBitsPerSample = 0;

	bool fileOpenedState = false;
	TPlayerStatus playerStatus = TPlayerStatus::NotOpen;



	int lastErrorCode = 0;
	std::wstring lastErrorMessage;
};



