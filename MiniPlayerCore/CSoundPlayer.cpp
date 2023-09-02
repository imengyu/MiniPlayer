#include "pch.h"
#include "CSoundPlayer.h"
#include "StringHelper.h"
#include "CAacDecoder.h"
#include "CFLACDecoder.h"
#include "CMP3Decoder.h"
#include "COggDecoder.h"
#include "CWavDecoder.h"
#include "CWmaDecoder.h"

extern TStreamFormat GetAudioFileFormat(const wchar_t* pchFileName);


bool CSoundPlayer::Load(const wchar_t* path)
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
bool CSoundPlayer::Close()
{
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


TStreamFormat CSoundPlayer::GetFormat() {
	return openedFileFormat;
}
CSoundDecoder* CSoundPlayer::CreateDecoderWithFormat(TStreamFormat f)
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

double CSoundPlayer::GetPosition()
{
	return GetPositionSample() / (double)currentSampleRate;
}
void CSoundPlayer::SetPosition(double second)
{
	SetPositionSample(second * currentSampleRate);
}
double CSoundPlayer::GetDuration()
{
	if (decoder)
		return decoder->GetLengthSecond();
	return 0.0;
}
unsigned int CSoundPlayer::GetDurationSample()
{
	if (decoder)
		return decoder->GetLengthSample();
	return 0.0;
}

int CSoundPlayer::GetVolume()
{
	return 0;
}

void CSoundPlayer::SetLastError(int code, const wchar_t* errmsg)
{
	lastErrorMessage = errmsg ? errmsg : L"";
	lastErrorCode = code;
}
void CSoundPlayer::SetLastError(int code, const char* errmsg)
{
	lastErrorMessage = errmsg ? StringHelper::AnsiToUnicode(errmsg) : L"";
	lastErrorCode = code;
}
const wchar_t* CSoundPlayer::GetLastErrorMessage()
{
	return lastErrorMessage.c_str();
}


