#include "pch.h"
#include "CSoundPlayer.h"
#include "CMP3Decoder.h"
#include "COggDecoder.h"
#include "CWavDecoder.h"
#include "StringHelper.h"
#include "CCVideoPlayer.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

#pragma region Player creation

CSoundPlayer* CreateSoundPlayer()
{
   return new CSoundPlayerImpl();
}
void DestroySoundPlayer(CSoundPlayer* player)
{
	delete player;
}

CCVideoPlayerAbstract* CreateVideoPlayer(CCVideoPlayerInitParams* param)
{
	return new CCVideoPlayer(param);
}
void DestroyVideoPlayer(CCVideoPlayerAbstract* player)
{
	delete player;
}

#pragma endregion

#pragma region Audio utils

#define RETURN_TYPE(type) { fclose(f);return type; }

TStreamFormat GetAudioFileFormat(const wchar_t* pchFileName)
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

double GetAudioDurationFast(const wchar_t* file)
{
	auto type = GetAudioFileFormat(file);
	switch (type)
	{
	case sfMp3: {
		int ret = -1;
		auto _handle = mpg123_new(NULL, &ret);
		if (_handle == NULL || ret != MPG123_OK)
			return false;

		auto str = StringHelper::UnicodeToAnsi(file);
		if (mpg123_open(_handle, str.c_str()) != MPG123_OK) {
			mpg123_delete(_handle);
			return 0;
		}

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
			{
				vorbis_info* pInfo = NULL;
				pInfo = ov_info(&_OggFile, -1);
				if (pInfo == NULL)
					goto FAIL_CLEAN_ALL;
				auto m_SampleRate = static_cast<ULONG>(pInfo->rate);
				auto m_Samples = ov_pcm_total(&_OggFile, -1);
				return static_cast<double>(m_Samples) / (double)m_SampleRate;
			}
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

bool GetAllAudioOutDeviceInfo(CSoundDeviceAudioOutDeviceInfo** outList, int* outCount)
{
	return CSoundDevice::GetAllAudioOutDeviceInfo(outList, outCount);
}

void DeleteAllAudioOutDeviceInfo(CSoundDeviceAudioOutDeviceInfo** ptr) 
{
	CSoundDevice::DeleteAllAudioOutDeviceInfo(ptr);
}

#pragma endregion

#pragma region Video utils

void ReleaseVideoInfo(READ_VIDEO_INFO* ptr) {
	delete ptr;
}

READ_VIDEO_INFO* GetVideoInfo(const wchar_t* pchFileName) {
	READ_VIDEO_INFO* result = new READ_VIDEO_INFO();
	result->success = false;

	int videoIndex = -1;
	int audioIndex = -1;
	auto formatContext = avformat_alloc_context();
	auto ansiPath = StringHelper::UnicodeToUtf8(pchFileName);
	AVStream* videoStream;
	AVStream* audioStream;

	//打开视频数据源
	int openState = avformat_open_input(&formatContext, ansiPath.c_str(), nullptr, nullptr);
	if (openState < 0) {
		char errBuf[128];
		if (av_strerror(openState, errBuf, sizeof(errBuf)) == 0)
			swprintf_s(result->lastError, L"Failed to open input file, error : %hs", errBuf);
		goto EXIT;
	}
	//为分配的AVFormatContext 结构体中填充数据
	if (avformat_find_stream_info(formatContext, nullptr) < 0) {
		swprintf_s(result->lastError, L"Failed to read the input video stream information");
		goto EXIT;
	}

	//找到对应的流
	for (int i = 0; i < (int)formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoIndex = i;
			break;
		}
	}
	for (int i = 0; i < (int)formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioIndex = i;
			break;
		}
	}
	if (videoIndex == -1) {
		swprintf_s(result->lastError, L"Not found video stream!");
		goto EXIT;
	}

	videoStream = formatContext->streams[videoIndex];
	audioStream = formatContext->streams[audioIndex];

	result->width = videoStream->codecpar->width;
	result->height = videoStream->codecpar->height;
	result->duration = (double)formatContext->duration / AV_TIME_BASE; //s
	result->frameRate = (double)videoStream->r_frame_rate.num / videoStream->r_frame_rate.den;
	strcpy_s(result->format, formatContext->iformat->long_name);
	result->success = true;

EXIT:
	avformat_close_input(&formatContext);
	avformat_free_context(formatContext);

	return result;
}

#pragma endregion
