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

CM_API_RESULT* ReportCmSuccess(void* data) {
	auto result = new CM_API_RESULT();
	result->Success = true;
	result->Result = data;
	return result;
}
CM_API_RESULT* ReportCmFailure(const char* errMessage) {
	auto result = new CM_API_RESULT();
	result->Success = false;
	strcpy_s(result->ErrorMessage, errMessage);
	return result;
}
CM_API_RESULT* ReportCmAvFailure(const char* errMessage, int ret) {
	char error[64];
	char averror[32];
	av_make_error_string(averror, sizeof(averror), ret);
	sprintf_s(error, errMessage, averror);
	return ReportCmFailure(error);
}

CM_API_RESULT* FloatPCMArrayToWavFile(const char* output_file, float* pcm_data, long pcm_count, int sample_rate, int channels, double sacle_to_second, double sacle_start_space) {
	AVFormatContext* output_ctx = nullptr;
	AVCodecContext* codec_ctx = nullptr;
	const AVCodec* codec = nullptr;
	AVFrame* frame = nullptr;
	int ret;

	// 创建输出WAV文件上下文
	if ((ret = avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, output_file)) < 0)
		return ReportCmAvFailure("Could not create output context: %s", ret);

	// 查找输出格式
	const AVOutputFormat* output_format = output_ctx->oformat;

	// 设置音频流信息
	AVStream* stream = avformat_new_stream(output_ctx, nullptr);
	if (!stream)
		return ReportCmFailure("Could not create stream");

	AVCodecParameters* codec_params = stream->codecpar;
	codec_params->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_params->codec_id = output_format->audio_codec;

	const int nb_channels = channels;
	const int nb_samples = 1;

	AVChannelLayout dst_ch_layout;
	dst_ch_layout.nb_channels = nb_channels;
	dst_ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
	dst_ch_layout.u.mask = nb_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;

	codec_params->ch_layout = dst_ch_layout;  // 设置声道布局
	codec_params->sample_rate = sample_rate;  // 设置采样率
	codec_params->format = AV_SAMPLE_FMT_S16;  // 设置采样格式

	// 查找编码器
	codec = avcodec_find_encoder(codec_params->codec_id);
	if (!codec)
		return ReportCmFailure("Could not find encoder");

	// 创建编码器上下文
	codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx)
		return ReportCmAvFailure("Could not allocate codec context: %s", ret);

	codec_ctx->time_base = av_get_time_base_q();

	// 设置编码器上下文参数
	if ((ret = avcodec_parameters_to_context(codec_ctx, codec_params)) < 0)
		return ReportCmAvFailure("Could not set codec parameters: %s", ret);

	// 打开编码器
	if ((ret = avcodec_open2(codec_ctx, codec, nullptr)) < 0) 
		return ReportCmAvFailure("Could not open codec:  %s", ret);

	// 打开输出文件
	if (!(output_format->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&output_ctx->pb, output_file, AVIO_FLAG_WRITE)) < 0) 
			return ReportCmAvFailure("Could not open output file: %s", ret);
	}

	// 写文件头部信息
	if ((ret = avformat_write_header(output_ctx, nullptr)) < 0)
		return ReportCmAvFailure("Error writing file header: %s", ret);

	// 分配帧内存
	frame = av_frame_alloc();
	if (!frame)
		return ReportCmFailure("Could not allocate frame");

	// 设置帧的格式和采样率
	frame->format = codec_ctx->sample_fmt;
	frame->sample_rate = codec_ctx->sample_rate;
	frame->ch_layout = dst_ch_layout;

	// 设置帧数据大小
	frame->nb_samples = nb_samples;
	int frame_size = av_samples_get_buffer_size(nullptr, nb_channels, nb_samples, codec_ctx->sample_fmt, 0);
	uint8_t** frame_data = NULL;
	int input_linesize;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0)
		return ReportCmAvFailure("Could not allocate frame buffer: %s", ret);

	// 给pcm文件数据分配空间
	ret = av_samples_alloc_array_and_samples(&frame_data, &input_linesize, nb_channels, nb_samples, codec_ctx->sample_fmt, 0);
	if (ret < 0)
		return ReportCmAvFailure("Could not allocate sample buffer: %s", ret);

	AVPacket* avPacket = av_packet_alloc();
	if (!avPacket)
		return ReportCmFailure("Could not allocate packet");

	int64_t pts = 0;
	int64_t pcmPos = 0, pcmCount = pcm_count, pcmStartPos = 0;
	if (sacle_to_second > 0) {
		pcmCount = (int64_t)(sacle_to_second * (double)sample_rate * (double)nb_channels);
		pcmStartPos = (int64_t)(sacle_start_space * (double)sample_rate * (double)nb_channels);
	}

	while (pcmPos < pcmCount)
	{
		//拷贝数据至frame
		for (size_t i = 0; i < nb_channels * nb_samples; i++) {
			if (pcmStartPos > 0) {
				pcmStartPos--;
				((int16_t*)frame_data[0])[i] = 0;
			}
			else {
				pcmPos++;
				((int16_t*)frame_data[0])[i] = static_cast<int16_t>(pcmPos < pcm_count ? pcm_data[pcmPos] * INT16_MAX : 0);
			}
		}

		frame->pts = pts;
		avPacket->pts = pts;

		// 计算PTS
		double frame_duration = (double)nb_samples / sample_rate;
		int64_t frame_pts = (int64_t)(frame_duration * AV_TIME_BASE);
		pts += frame_pts;

		frame->data[0] = frame_data[0];

		// 编码帧
		ret = avcodec_send_frame(codec_ctx, frame);
		if (ret < 0)
			return ReportCmAvFailure("Error sending frame for encoding: %s", ret);

		while (ret >= 0) {
			// 接收编码后的数据包
			ret = avcodec_receive_packet(codec_ctx, avPacket);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
				return ReportCmAvFailure("Error during encoding: %s", ret);

			// 将数据包写入输出文件
			ret = av_interleaved_write_frame(output_ctx, avPacket);
			if (ret < 0)
				return ReportCmAvFailure("Error writing packet to file: %s", ret);

			// 释放数据包
			av_packet_unref(avPacket);
		}
	}


	// 写文件尾部信息
	av_write_trailer(output_ctx);

	// 清理资源
	av_frame_free(&frame);
	av_packet_free(&avPacket);
	avcodec_free_context(&codec_ctx);
	avformat_close_input(&output_ctx);
	avformat_free_context(output_ctx);

	return ReportCmSuccess(nullptr);
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
	FILE* fileStream = nullptr;

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
	if (videoIndex >= 0) {
		videoStream = formatContext->streams[videoIndex];

		result->width = videoStream->codecpar->width;
		result->height = videoStream->codecpar->height;
		result->frameRate = (double)videoStream->r_frame_rate.num / videoStream->r_frame_rate.den;
		result->avgFrameRate = (double)videoStream->avg_frame_rate.num / videoStream->avg_frame_rate.den;
		result->isVideo = true;

	}
	if (audioIndex >= 0) {

		audioStream = formatContext->streams[audioIndex];
		result->simpleRate = (double)audioStream->r_frame_rate.num / audioStream->r_frame_rate.den;
		result->channels = audioStream->codecpar->ch_layout.nb_channels;
		result->bitRate = (double)audioStream->codecpar->bit_rate;
		result->bitsPreSample = audioStream->codecpar->bits_per_raw_sample;
		result->isAudio = true;
	}
	
	result->duration = (double)formatContext->duration / AV_TIME_BASE; //s
	sprintf_s(result->format, "%s %s", formatContext->iformat->name, formatContext->iformat->long_name);
	result->success = true;


EXIT:

	avformat_close_input(&formatContext);
	avformat_free_context(formatContext);


	//获取文件大小
	_wfopen_s(&fileStream, pchFileName, L"r");
	if (fileStream) {
		fseek(fileStream, 0, SEEK_END);
		result->size = (int64_t)ftell(fileStream);
		fclose(fileStream);
	}

	return result;
}

#pragma endregion

void DeletePlayerMemory(void* ptr) {
	if (ptr)
		delete ptr;
}