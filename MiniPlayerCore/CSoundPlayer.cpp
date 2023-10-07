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
	if (fileOpenedState)
		Close();

	TStreamFormat thisFileFormat = GetAudioFileFormat(path);
	if (thisFileFormat == TStreamFormat::sfUnknown) {
		SetLastError(PLAYER_ERROR_UNKNOWN_FILE_FORMAT, L"UNKNOWN_FILE_FORMAT");
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
			SetLastError(
				PLAYER_ERROR_DECODER_ERROR, 
				StringHelper::FormatString(L"Decoder error: %d message: %s", decoder->GetLastError(), decoder->GetLastErrorMessage()).c_str()
			);
		}
	}
	return false;
}
bool CSoundPlayerImpl::Close()
{
	if (outputer != NULL) {
		outputer->Destroy();
	}
	if (decoder != NULL) {
		decoder->Close();
		delete decoder;
		decoder = NULL;
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
	if (playerStatus == Playing) {
		return true;
	}
	bool rs = outputer->Start();
	if (rs)
		playerStatus = TPlayerStatus::Playing;
	else
		Close();
	return rs;
}
bool CSoundPlayerImpl::Pause()
{
	if (playerStatus == NotOpen) {
		SetLastError(PLAYER_ERROR_NOT_LOAD, L"No audio loaded in this player");
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
	auto read_size = dynamic_cast<CSoundPlayerImpl*>(instance)->decoder->Read(buf, buf_len);
	if (read_size < buf_len) {
		memset((void*)((size_t)buf + read_size), 0, buf_len - read_size);
		return false;
	}
	return true;
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

void CSoundPlayerImpl::NotifyPlayEnd(bool error)
{
	if (error)
		Close();
	else if (playerStatus != PlayEnd)
		playerStatus = PlayEnd;
}


