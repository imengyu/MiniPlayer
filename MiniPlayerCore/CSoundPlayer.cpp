#include "pch.h"
#include "CSoundPlayer.h"
#include "StringHelper.h"
#include "CAacDecoder.h"
#include "CFLACDecoder.h"
#include "CMP3Decoder.h"
#include "COggDecoder.h"
#include "CWavDecoder.h"
#include "CWmaDecoder.h"

CSoundPlayerImpl::CSoundPlayerImpl()
{
	outputer = new CSoundDevice(this);
	outputer->SetOnCopyDataCallback(OnCopyData);
}
CSoundPlayerImpl::~CSoundPlayerImpl()
{
	if (playerStatus == Playing)
		Stop();

	Close();
	if (outputer)
		delete outputer;
}

bool CSoundPlayerImpl::Load(const wchar_t* path)
{
	if (playerStatus == Loading) {
		SetLastError(PLAYER_ERROR_LOADING, L"Load: Player is now loading");
		return false;
	}
	if (fileOpenedState)
		Close();
	if (preloadReadyState)
		preloadReadyState = false;

	playerStatus = TPlayerStatus::Loading;

	TStreamFormat thisFileFormat = GetAudioFileFormat(path);
	if (thisFileFormat == TStreamFormat::sfUnknown) {
		SetLastError(PLAYER_ERROR_UNKNOWN_FILE_FORMAT, L"UNKNOWN_FILE_FORMAT");
		playerStatus = TPlayerStatus::NotOpen;
		return false;
	}

	if (openedFileFormat != thisFileFormat) {
		if (decoder)
		{
			if (decoder->IsOpened())
				decoder->Close();
			decoder->Release();
			decoder = nullptr;
		}
	}
	if (decoder == nullptr)
		decoder = CreateDecoderWithFormat(thisFileFormat);
	if (decoder != nullptr) {
		openedFileFormat = thisFileFormat;
		if (decoder->Open(path)) {
			decoder->SeekToSecond(0);
			playerStatus = TPlayerStatus::Opened;

			currentSampleRate = decoder->GetSampleRate();
			currentChannels = decoder->GetChannelsCount();
			currentBitsPerSample = decoder->GetBitsPerSample();

			if (!outputer->Create())
				return false;

#if _DEBUG
			OutputDebugString(StringHelper::FormatString(L"duration : %f sample : %d sample_rate: %d channel: %d bits_per_sample: %d\n",
				decoder->GetLengthSecond(),
				decoder->GetLengthSample(),
				decoder->GetSampleRate(),
				decoder->GetChannelsCount(),
				decoder->GetBitsPerSample()
			).c_str());
#endif

			return true;
		}
		else {
			playerStatus = TPlayerStatus::NotOpen;
			SetLastError(
				PLAYER_ERROR_DECODER_ERROR, 
				StringHelper::FormatString(L"Decoder error: %d message: %s", decoder->GetLastError(), decoder->GetLastErrorMessage()).c_str()
			);
		}
	}
	return false;
}
bool CSoundPlayerImpl::PreLoad(const wchar_t* path)
{
	//不可以重复加载
	if (preloadStatus == TPlayerStatus::Loading) {
		SetLastError(PLAYER_ERROR_LOADING, "Threre are preload loading, please wait");
		return false;
	}

	preloadStatus = TPlayerStatus::Loading;

	//如果播放器处于未加载状态，则直接调用正常的加载方法，无需预加载
	if (playerStatus == TPlayerStatus::NotOpen || playerStatus == TPlayerStatus::PlayEnd) {
		preloadReadyState = Load(path);
		preloadStatus = preloadReadyState ? TPlayerStatus::Opened : TPlayerStatus::NotOpen;
		return preloadReadyState;
	}

	preloadReadyState = false;

	if (preloadDecoder)
		preloadDecoder->Close();

	TStreamFormat thisFileFormat = GetAudioFileFormat(path);
	if (thisFileFormat == TStreamFormat::sfUnknown) {
		SetLastError(PLAYER_ERROR_UNKNOWN_FILE_FORMAT, L"Unknown file format");
		preloadStatus = TPlayerStatus::NotOpen;
		return false;
	}

	//如果之前的预加载解码器与当前格式不一致，则销毁之前的，重新创建
	if (preloadDecoder && preloadDecoder->GetFormat() != thisFileFormat) {
		delete preloadDecoder;
		preloadDecoder = nullptr;
	}

	if (!preloadDecoder)
		preloadDecoder = CreateDecoderWithFormat(thisFileFormat);
	if (!preloadDecoder) {
		preloadStatus = TPlayerStatus::NotOpen;
		return false;
	}

	if (preloadDecoder->Open(path)) {
		preloadReadyState = true;
		preloadStatus = TPlayerStatus::Opened;
		return true;
	}

	preloadStatus = TPlayerStatus::NotOpen;
	return false;
}
bool CSoundPlayerImpl::Close()
{
	if (playerStatus == Loading) {
		SetLastError(PLAYER_ERROR_LOADING, L"Close: Player is now loading");
		return false;
	}
	if (playerStatus == Closing)
		return true;
	playerStatus = Closing;
	if (outputer) {
		outputer->Stop();
		outputer->Destroy();
	}
	if (preloadDecoder) {
		preloadDecoder->Close();
		delete preloadDecoder;
		preloadDecoder = nullptr;
	}
	if (decoder != nullptr) {
		decoder->Close();
		delete decoder;
		decoder = nullptr;
	}
	playerStatus = NotOpen;
	openedFileFormat = TStreamFormat::sfUnknown;
	fileOpenedState = false;
	return true;
}

bool CSoundPlayerImpl::Play()
{
	if (playerStatus == NotOpen) {
		SetLastError(PLAYER_ERROR_NOT_LOAD, L"No audio loaded in this player");
		return false;
	}
	if (playerStatus == Loading) {
		SetLastError(PLAYER_ERROR_LOADING, L"Play: Player is now loading");
		return false;
	}
	if (playerStatus == Playing) {
		return true;
	}
	playerStatus = TPlayerStatus::Loading;
	bool rs = outputer->Start();
	if (rs)
		playerStatus = TPlayerStatus::Playing;
	else 
		playerStatus = TPlayerStatus::Opened;
	return rs;
}
bool CSoundPlayerImpl::Pause()
{
	if (playerStatus == NotOpen) {
		SetLastError(PLAYER_ERROR_NOT_LOAD, L"No audio loaded in this player");
		return false;
	}
	if (playerStatus == Loading) {
		SetLastError(PLAYER_ERROR_LOADING, L"Pause: Player is now loading");
		return false;
	}
	if (playerStatus == Paused) {
		return true;
	}
	playerStatus = TPlayerStatus::Paused;
	outputer->Stop();
	return true;
}
bool CSoundPlayerImpl::Stop()
{
	if (playerStatus == NotOpen) {
		SetLastError(PLAYER_ERROR_NOT_LOAD, L"No audio loaded in this player");
		return false;
	}
	if (playerStatus == TPlayerStatus::Opened)
		return true;
	outputer->Stop();
	playerStatus = TPlayerStatus::Opened;
	return true;
}
bool CSoundPlayerImpl::Restart()
{
	if (playerStatus == Loading) {
		SetLastError(PLAYER_ERROR_LOADING, L"Restart: Player is now loading");
		return false;
	}
	if (playerStatus != NotOpen) {
		decoder->SeekToSecond(0);
		outputer->Reset();
		return true;
	}
	else {
		SetLastError(PLAYER_ERROR_NOT_LOAD, L"No audio loaded in this player");
	}
	return false;
}

double CSoundPlayerImpl::GetPosition()
{
	return GetPositionSample() / (double)currentSampleRate;
}
void CSoundPlayerImpl::SetPosition(double second)
{
	SetPositionSample((unsigned int)(second * currentSampleRate));
}
double CSoundPlayerImpl::GetDuration()
{
	if (decoder)
		return decoder->GetLengthSecond();
	return 0.0;
}
bool CSoundPlayerImpl::IsPreLoad()
{
	return preloadReadyState;
}
unsigned int CSoundPlayerImpl::GetDurationSample()
{
	if (decoder)
		return decoder->GetLengthSample();
	return 0;
}
unsigned int CSoundPlayerImpl::GetPositionSample()
{
	if (decoder)
		return decoder->GetCurrentPositionSample();
	return 0;
}
void CSoundPlayerImpl::SetPositionSample(unsigned int sample)
{
	if (playerStatus != NotOpen) {
		if (sample != decoder->GetCurrentPositionSample()) {
			if (playerStatus == Playing)
				outputer->Stop();
			decoder->SeekToSample(sample);
			if (playerStatus == Playing)
				outputer->Start();
		}
	}
}

bool CSoundPlayerImpl::OnCopyData(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len, DWORD sample)
{
	auto player = dynamic_cast<CSoundPlayerImpl*>(instance);
	auto read_size = player->decoder->Read(buf, buf_len);
	if (read_size < buf_len) {
		if (player->IsPreLoad() && player->preloadDecoder)
			//有预加载，则直接切换预加载加载这一段空数据
			player->preloadDecoder->Read((void*)((size_t)buf + read_size), buf_len - read_size);
		else 
			memset((void*)((size_t)buf + read_size), 0, buf_len - read_size);
		return false;
	}
	return true;
}

void CSoundPlayerImpl::CallEventCallback(int event)
{
	if (eventCallback)
		eventCallback(this, event, eventCallbackCustomData);
}
void CSoundPlayerImpl::SetEventCallback(CSoundPlayerEventCallback callback, void* customData)
{
	eventCallback = callback;
	eventCallbackCustomData = customData;
}

void CSoundPlayerImpl::SetDefaultOutputDeviceId(const wchar_t* deviceId)
{
	defaultOutputDeviceId = deviceId;
}
const wchar_t* CSoundPlayerImpl::GetDefaultOutputDeviceId()
{
	return defaultOutputDeviceId.c_str();
}

void CSoundPlayerImpl::SetLastError(int code, const wchar_t* errmsg)
{
	lastErrorMessage = errmsg ? errmsg : L"";
	lastErrorCode = code;
}
void CSoundPlayerImpl::SetLastError(int code, const char* errmsg)
{
	lastErrorMessage = errmsg ? StringHelper::AnsiToUnicode(errmsg) : L"";
	lastErrorCode = code;
}
const wchar_t* CSoundPlayerImpl::GetLastErrorMessage()
{
	return lastErrorMessage.c_str();
}
const char* CSoundPlayerImpl::GetLastErrorMessageUtf8() {
	lastErrorMessageUtf8 = StringHelper::UnicodeToUtf8(lastErrorMessage);
	return lastErrorMessageUtf8.c_str();
}

void CSoundPlayerImpl::SetVolume(float volume, int index)
{
	if (outputer)
		outputer->SetVolume(index, volume);
}
float CSoundPlayerImpl::GetVolume(int index)
{
	if (outputer)
		return outputer->GetVolume(index);
	return 0.0f;
}

TPlayerStatus CSoundPlayerImpl::GetState()
{
	return playerStatus;
}
TStreamFormat CSoundPlayerImpl::GetFormat() {
	return openedFileFormat;
}
CSoundDecoder* CSoundPlayerImpl::CreateDecoderWithFormat(TStreamFormat f)
{
	switch (f)
	{
	case sfMp3:
		return new CMP3Decoder();
	case sfOgg:
		return new COggDecoder();
	case sfWav:
		return new CWavDecoder();
	case sfWma:
		return new CWmaDecoder();
	case sfFLAC:
		return new CFLACDecoder(false);
	case sfFLACOgg:
		return new CFLACDecoder(true);
	case sfAacADTS:
		return new CAacDecoder();
	default:
		SetLastError(PLAYER_ERROR_NOT_SUPPORT_FORMAT, L"Unsupport file format.");
		break;
	}
	return nullptr;
}

CSoundDevicePreloadType CSoundPlayerImpl::PlayAlmostEndAndCheckPrelod()
{
	//播放即将结束，现在检查是否可以预加载
	if (
		preloadReadyState && decoder != preloadDecoder && 
		preloadDecoder && preloadDecoder->IsOpened()
	) {

		//如果音频参数与之前的不一致，则需要重新启动音频线程
		bool needRecreatePlay = decoder->GetBitsPerSample() != preloadDecoder->GetBitsPerSample() ||
			decoder->GetSampleRate() != preloadDecoder->GetSampleRate() ||
			decoder->GetChannelsCount() != preloadDecoder->GetChannelsCount();

		//交换preloadDecoder和decoder，保证两个都可以释放
		auto _preloadDecoder = preloadDecoder;

		if (decoder) {
			decoder->Close();
			preloadDecoder = decoder;
		}

		//可用预加载解码器，则直接换到此解码器
		decoder = _preloadDecoder;
		preloadReadyState = false; //已使用预加载

		CallEventCallback(SOUND_PLAYER_EVENT_PLAY_END);
		return needRecreatePlay ? CSoundDevicePreloadType::PreloadReload : CSoundDevicePreloadType::PreloadSame;
	}
	return CSoundDevicePreloadType::NoPreload;
}
void CSoundPlayerImpl::NotifyPlayEnd(bool error)
{
	if (error) {
		Close();
		CallEventCallback(SOUND_PLAYER_EVENT_PLAY_ERROR);
	}
	else if (playerStatus != PlayEnd) {
		playerStatus = PlayEnd;
		CallEventCallback(SOUND_PLAYER_EVENT_PLAY_END);
	}
}


