#pragma once
#include "CSoundDecoder.h"
#include "CSoundDevice.h"
#include "CSoundPlayerExport.h"
#include "Export.h"
#include <string>

//²¥·ÅÆ÷ÊµÀý
class CSoundPlayerImpl : public CSoundPlayer, public CSoundDeviceHoster
{
public:
	CSoundPlayerImpl();
	~CSoundPlayerImpl();

	bool Load(const wchar_t* path);
	bool PreLoad(const wchar_t* path);
	bool Close();
	bool Play();
	bool Pause();
	bool Stop();
	bool Restart();

	double GetPosition();
	void SetPosition(double second);
	double GetDuration();

	bool IsPreLoad();

	TPlayerStatus GetState();
	TStreamFormat GetFormat();

	void SetVolume(float volume, int index = 0);
	float GetVolume(int index = 0);

	void SetLastError(int code, const wchar_t* errmsg);
	void SetLastError(int code, const char* errmsg);
	int GetLastError() { return lastErrorCode; }
	const wchar_t* GetLastErrorMessage(); 
	const char* GetLastErrorMessageUtf8();

	unsigned long GetSampleRate() { return currentSampleRate; }
	int GetBitPerSample() { return currentBitsPerSample; }
	int GetChannelsCount() { return currentChannels; }
	const wchar_t* GetDefaultOutputDeviceId();

	unsigned int GetDurationSample();
	unsigned int GetPositionSample();
	void SetPositionSample(unsigned int sample);

	CSoundDevicePreloadType PlayAlmostEndAndCheckPrelod();
	void NotifyPlayEnd(bool error);
	bool GetShouldReSample() { return true; }

	void SetEventCallback(CSoundPlayerEventCallback callback, void* customData);

	void SetDefaultOutputDeviceId(const wchar_t* deviceId);
private:
	CSoundDecoder* CreateDecoderWithFormat(TStreamFormat f);
	static bool OnCopyData(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len, DWORD sample);

private:
	CSoundDecoder* decoder = nullptr;
	CSoundDevice* outputer = nullptr;
	CSoundDecoder* preloadDecoder = nullptr;
	TPlayerStatus preloadStatus = TPlayerStatus::NotOpen;

	TStreamFormat openedFileFormat = TStreamFormat::sfUnknown;

	void* eventCallbackCustomData;
	CSoundPlayerEventCallback eventCallback;

	void CallEventCallback(int event);

	ULONG currentSampleRate = 0;
	int currentChannels = 0;
	int currentBitsPerSample = 0;

	bool preloadReadyState = false;
	bool fileOpenedState = false;
	TPlayerStatus playerStatus = TPlayerStatus::NotOpen;

	int lastErrorCode = 0;
	std::wstring defaultOutputDeviceId;
	std::wstring lastErrorMessage;
	std::string lastErrorMessageUtf8;
};



