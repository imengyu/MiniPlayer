#include "Export.h"
#include "PlayerTest.h"
#include <Windows.h>
#include <Commdlg.h>  
#include <Shlobj.h>  
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class PlayVideoData {
public:
	SDL_Window* window = nullptr;
	SDL_Renderer* render = nullptr;
	SDL_Texture* texture = nullptr;
	SDL_Event event;
	SDL_Rect rect;
	bool quit = false;
	bool openSuccess = false;
};

void DoPlaySound(wchar_t* strFilename) {
	CSoundPlayer* player = CreateSoundPlayer();
	int count = 0;

	wprintf(L"音乐：%s\n", strFilename);
	if (player->Load(strFilename))
	{

		wprintf(L"音乐已打开\n");
		if (player->Play())
		{
			wprintf(L"音乐长度：%.3f 秒\n", player->GetDuration());
			wprintf(L"音乐开始播放\n正在播放：000.000/%7.3f", player->GetDuration());
			for (int i = 0; i < 8; i++)
				putchar('\b');

			auto state = player->GetState();
			while (state != TPlayerStatus::PlayEnd)
			{
				for (int i = 0; i < 7; i++)
					putchar('\b');
				wprintf(L"%7.3f", player->GetPosition());
				Sleep(250);

				if (state == TPlayerStatus::NotOpen) {
					wprintf(L"播放失败：%d %s\n", player->GetLastError(), player->GetLastErrorMessage());
					break;
				}

				state = player->GetState();
			}
		}
		else wprintf(L"播放失败：%d %s\n", player->GetLastError(), player->GetLastErrorMessage());

		player->Stop();
		player->Close();
		count++;

		wprintf(L"\n音乐已停止\n");
	}
	else {
		wprintf(L"打开文件失败：%d %s\n", player->GetLastError(), player->GetLastErrorMessage());
	}
	DestroySoundPlayer(player);
}

void DoPlayVideo(wchar_t* strFilename, READ_VIDEO_INFO*info) {
	PlayVideoData playData;
	CCVideoPlayerInitParams params;
	uint32_t _FPS_Timer = 0;
	const uint32_t FPS = (uint32_t)(1000.0 / (info->frameRate > 0 ? info->frameRate : 1));

	params.DestFormat = 0;//AV_PIX_FMT_YUV420P
	params.DestWidth = 408;
	params.DestHeight = 720;
	params.UseRenderCallback = false;
	params.UseHadwareDecoder = true;
	params.SyncRender = true;

	CCVideoPlayerAbstract* player = CreateVideoPlayer(&params);

	//播放器回调
	player->SetPlayerEventCallback([](CCVideoPlayerAbstract* player, int message, void* eventData, void* customData) {
		auto playData = (PlayVideoData*)(customData);
		switch (message)
		{
		case PLAYER_EVENT_CLOSED: {
			playData->openSuccess = false;
			break;
		}
		case PLAYER_EVENT_PLAY_END: {
			playData->openSuccess = false;
			break;
		}
		case PLAYER_EVENT_INIT_DECODER_DONE: {
			break;
		}
		default:
			break;
		}
		}, &playData);

	//打开视频
	if (!player->OpenVideo(strFilename)) {
		wprintf(L"OpenVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
		goto DESTROY;
	}

	//初始化环境
	SDL_Init(0);

	//创建一个窗口
	playData.window = SDL_CreateWindow(
		"PlayVideo",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		params.DestWidth,
		params.DestHeight,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
	);
	if (!playData.window)
	{
		wprintf(L"SDL_CreateWindow failed: %hs", SDL_GetError());
		goto DESTROY;
	}

	//创建一个渲染器
	playData.render = SDL_CreateRenderer(playData.window, -1, 0);
	if (!playData.render)
	{
		wprintf(L"SDL_CreateRenderer failed: %hs", SDL_GetError());
		goto DESTROY;
	}

	//创建一个文理
	playData.texture = SDL_CreateTexture(
		playData.render,
		SDL_PIXELFORMAT_IYUV,
		SDL_TEXTUREACCESS_TARGET,
		params.DestWidth,
		params.DestHeight
	);

	player->SetVideoState(CCVideoState::Playing);

	do
	{
		SDL_PollEvent(&playData.event);//轮训方式
		switch (playData.event.type)
		{
		case SDL_QUIT:
			playData.quit = true;
			break;
		case SDL_KEYDOWN:
			switch (playData.event.key.keysym.sym) {
			case SDLK_0: {
				int p = 1000;
				player->PostWorkerThreadCommand(VIDEO_PLAYER_ASYNC_TASK_SET_POS, (void*)p);
				break;
			}
			case SDLK_1:
				player->SetVideoPos(2000);
				break;
			case SDLK_2:
				player->SetVideoPos(3000);
				break;
			case SDLK_3:
				player->SetVideoPos(4000);
				break;
			case SDLK_4:
				player->SetVideoPos(5000);
				break;
			case SDLK_5:
				player->SetVideoPos(6000);
				break;
			}
			break;
		case SDL_WINDOWEVENT:
			if (playData.event.window.event == SDL_WINDOWEVENT_RESIZED)
				player->RenderUpdateDestSize(playData.event.window.data1, playData.event.window.data2);
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (SDL_BUTTON_LEFT == playData.event.button.button)
			{
				if (player->GetVideoState() == CCVideoState::Playing)
					player->SetVideoState(CCVideoState::Paused);
				else if (player->GetVideoState() == CCVideoState::Paused)
					player->SetVideoState(CCVideoState::Playing);
			}
			break;
		default:
			break;
		}

		//帧率限制
		if (SDL_GetTicks() - _FPS_Timer < FPS) {
			SDL_Delay(FPS - SDL_GetTicks() + _FPS_Timer);
		}
		_FPS_Timer = SDL_GetTicks();

		if (info->isVideo) {
			//渲染
			if (player->GetVideoState() == CCVideoState::Playing) {
				auto rdcData = player->SyncRenderStart();
				if (rdcData && rdcData->width > 0 && rdcData->height > 0) {
					SDL_UpdateYUVTexture(
						playData.texture,
						NULL,
						rdcData->data[0], rdcData->linesize[0],
						rdcData->data[1], rdcData->linesize[1],
						rdcData->data[2], rdcData->linesize[2]
					);
				}
				player->SyncRenderEnd();

				playData.rect.x = 0;
				playData.rect.y = 0;
				playData.rect.w = params.DestWidth;
				playData.rect.h = params.DestHeight;
				SDL_RenderClear(playData.render);
				SDL_RenderCopy(playData.render, playData.texture, nullptr, &playData.rect);//纹理拷贝到显卡渲染
				SDL_RenderPresent(playData.render);//开始渲染图像并拷贝到显示器
			}
		}
		if (player->GetVideoState() == CCVideoState::Ended)
			playData.quit = true;
	} while (!playData.quit);


DESTROY:
	if (player) {
		if (!player->CloseVideo())
			wprintf(L"CloseVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
		player->WaitCloseVideo();
		DestroyVideoPlayer(player);
	}
	if (playData.texture)
		SDL_DestroyTexture(playData.texture);//删除纹理
	if (playData.render)
		SDL_DestroyRenderer(playData.render);//删除渲染器
	if (playData.window)
		SDL_DestroyWindow(playData.window);//删除窗口

	SDL_Quit();//卸载环境
}

void DoReadVideo(wchar_t* strFilename) {
	auto info = GetVideoInfo(strFilename);

	if (info->success)
		DoPlayVideo(strFilename, info);
	else
		wprintf(L"GetVideoInfo Failed %s\n", info->lastError);

	ReleaseVideoInfo(info);
}

int RunPlayer(const char* path) {
	wchar_t strFilename[MAX_PATH] = { 0 };
	if (path) {
		wcscpy_s(strFilename, StringHelper::AnsiToUnicode(path).c_str());
	}
	else {
		OPENFILENAME ofn = { 0 };
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = GetConsoleWindow();
		ofn.lpstrFilter = TEXT("音乐或者视频文件\0;*.mkv;*.mp4;*.mp3;*.wav;*.wma;*.ogg;*.flac;*.aac\0All(*.*)\0*.*\0\0\0");//设置过滤
		ofn.nFilterIndex = 1;//过滤器索引
		ofn.lpstrFile = strFilename;
		ofn.nMaxFile = sizeof(strFilename);
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = TEXT("打开音乐");
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
		if (!GetOpenFileName(&ofn))
		{
			wprintf(L"请选择一个文件\n");
			system("PAUSE");
			return 0;
		}
	}
	if (
		StringHelper::EndWith(strFilename, L".mp3") ||
		StringHelper::EndWith(strFilename, L".wav") ||
		StringHelper::EndWith(strFilename, L".ogg") ||
		StringHelper::EndWith(strFilename, L".flac") ||
		StringHelper::EndWith(strFilename, L".wma") ||
		StringHelper::EndWith(strFilename, L".aac")
	) {
		DoPlaySound(strFilename);
	}
	else {
		DoReadVideo(strFilename);
	}
	return 0;
}

int TestPCMToWav() {
	std::vector<float> array;

	FILE* fp = NULL;
	char str[32];
	_wfopen_s(&fp, L"C:\\Users\\Roger\\Desktop\\1.txt", L"r");
	if (fp) {
		while (!feof(fp))
		{
			if (fgets(str, 32, fp) != NULL)
				array.push_back((float)atof(str));
		}
		fclose(fp);
	}

	AVFormatContext* output_ctx = nullptr;
	AVCodecContext* codec_ctx = nullptr;
	const AVCodec* codec = nullptr;
	AVFrame* frame = nullptr;
	int ret;

	const char* output_file = "C:\\Users\\Roger\\Desktop\\output.wav";

	// 创建输出WAV文件上下文
	if ((ret = avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, output_file)) < 0) {
		printf("Could not create output context: %d", ret);
		return ret;
	}

	// 查找输出格式
	const AVOutputFormat* output_format = output_ctx->oformat;

	// 设置音频流信息
	AVStream* stream = avformat_new_stream(output_ctx, nullptr);
	if (!stream) {
		printf("Could not create stream");
		return AVERROR_UNKNOWN;
	}

	AVCodecParameters* codec_params = stream->codecpar;
	codec_params->codec_type = AVMEDIA_TYPE_AUDIO;
	codec_params->codec_id = output_format->audio_codec;

	const int nb_channels = 1;
	const int nb_samples = 1;
	const int sample_rate = 44100;

	AVChannelLayout dst_ch_layout;
	dst_ch_layout.nb_channels = nb_channels;
	dst_ch_layout.order = AV_CHANNEL_ORDER_NATIVE;
	dst_ch_layout.u.mask = nb_channels == 1 ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;

	codec_params->ch_layout = dst_ch_layout;  // 设置声道布局
	codec_params->sample_rate = sample_rate;  // 设置采样率
	codec_params->format = AV_SAMPLE_FMT_S16;  // 设置采样格式

	// 查找编码器
	codec = avcodec_find_encoder(codec_params->codec_id);
	if (!codec) {
		printf("Could not find encoder");
		return AVERROR_INVALIDDATA;
	}

	// 创建编码器上下文
	codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx) {
		printf("Could not allocate codec context: %d", ret);
		return AVERROR(ENOMEM);
	}

	codec_ctx->time_base = av_get_time_base_q();

	// 设置编码器上下文参数
	if ((ret = avcodec_parameters_to_context(codec_ctx, codec_params)) < 0) {
		printf("Could not set codec parameters:  %d", ret);
		return ret;
	}

	// 打开编码器
	if ((ret = avcodec_open2(codec_ctx, codec, nullptr)) < 0) {
		printf("Could not open codec:  %d", ret);
		return ret;
	}

	// 打开输出文件
	if (!(output_format->flags & AVFMT_NOFILE)) {
		if ((ret = avio_open(&output_ctx->pb, output_file, AVIO_FLAG_WRITE)) < 0) {
			printf("Could not open output file: %d", ret);
			return ret;
		}
	}

	// 写文件头部信息
	if ((ret = avformat_write_header(output_ctx, nullptr)) < 0) {
		printf("Error writing file header:  %d", ret);
		return ret;
	}

	// 分配帧内存
	frame = av_frame_alloc();
	if (!frame) {
		printf("Could not allocate frame");
		return AVERROR(ENOMEM);
	}

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
	{
		printf("Could not allocate frame");
		return ret;
	}

	// 给pcm文件数据分配空间
	ret = av_samples_alloc_array_and_samples(&frame_data, &input_linesize, nb_channels, nb_samples, codec_ctx->sample_fmt, 0);
	if (ret < 0)
	{
		printf("Could not allocate frame");
		return ret;
	}
	
	AVPacket* avPacket = av_packet_alloc();
	if (!avPacket)
	{
		printf("Could not allocate packet");
		return -1;
	}

	int64_t pts = 0;
	int pcmPos = 0, pcmCount = array.size();
	while (pcmPos < pcmCount)
	{
		for (size_t i = 0; i < nb_channels * nb_samples; i++) {
			pcmPos++;
			if (pcmPos < pcmCount)
				((int16_t*)frame_data[0])[i] = static_cast<int16_t>(array[pcmPos] * INT16_MAX);
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
		if (ret < 0) {
			char err[32];
			av_make_error_string(err, sizeof(err), ret);
			printf("Error sending frame for encoding:  %s", err);
			return ret;
		}

		while (ret >= 0) {
			// 接收编码后的数据包
			ret = avcodec_receive_packet(codec_ctx, avPacket);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				break;
			}
			else if (ret < 0) {
				printf("Error during encoding:  %d", ret);
				return ret;
			}

			// 将数据包写入输出文件
			ret = av_interleaved_write_frame(output_ctx, avPacket);
			if (ret < 0) {
				printf("Error writing packet to file: %d", ret);
				return ret;
			}

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

	return 0;
}