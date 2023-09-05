#pragma once
#include "CSoundDecoder.h"
#include "CSoundDevice.h"
#include "CSoundPlayerExport.h"
#include "Export.h"
#include <string>

//²¥·ÅÆ÷ÊµÀý
class CSoundPlayerImpl : public CSoundPlayer
{
public:
	CSoundPlayerImpl();
	~CSoundPlayerImpl();

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

	void SetVolume(float volume, int index = 0);
	float GetVolume(int index = 0);

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

	void NotifyPlayEnd(bool error);
private:
	CSoundDecoder* CreateDecoderWithFormat(TStreamFormat f);
	static bool OnCopyData(CSoundPlayerImpl* instance, LPVOID buf, DWORD buf_len);

private:
	CSoundDecoder* decoder = nullptr;
	CSoundDevice* outputer = nullptr;

	TStreamFormat openedFileFormat = TStreamFormat::sfUnknown;
	
	ULONG currentSampleRate = 0;
	int currentChannels = 0;
	int currentBitsPerSample = 0;

	bool fileOpenedState = false;
	TPlayerStatus playerStatus = TPlayerStatus::NotOpen;

	int lastErrorCode = 0;
	std::wstring lastErrorMessage;
};



