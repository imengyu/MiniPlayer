// MiniPlayer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Export.h"
#include <Windows.h>
#include <Commdlg.h>  
#include <Shlobj.h>  
#include <conio.h>  
#include <locale>
#include <thread>
#include <mutex>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <nanomsg/pair.h>
#include <nanomsg/bus.h>
#include <nanomsg/nn.h>
#pragma comment(lib,"Shell32.lib")  
#pragma comment(lib,"MiniPlayerCore.lib")  
#pragma comment(lib,"SDL2.lib")  
#pragma comment(lib,"SDL2main.lib")
#pragma comment(lib,"nanomsg.lib")

const wchar_t* AppTitle = L"播放器";
const char* SocketUrl = "tcp://127.0.0.1:4173";
const int SocketBufferSize = 256;

void DoPlaySound(wchar_t* strFilename) {
	CSoundPlayer* player = CreateSoundPlayer();
	int count = 0;

RELOAD:
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

		if (count == 1) {
			wcscpy_s(strFilename, 260, L"D:\\小语资源库\\主流程1\\新欢迎2\\新欢迎2.MP3");
			goto RELOAD;
		} else if (count == 2) {
			wcscpy_s(strFilename, 260, L"D:\\小语资源库\\主流程1\\新欢迎3\\新欢迎3.MP3");
			goto RELOAD;
		}


		wprintf(L"\n音乐已停止\n");
	}
	else {
		wprintf(L"打开文件失败：%d %s\n", player->GetLastError(), player->GetLastErrorMessage());
	}
	DestroySoundPlayer(player);
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

void DoPlayVideo(wchar_t* strFilename, int width, int height, int fps) {
	PlayVideoData playData;
	CCVideoPlayerInitParams params;
	uint32_t _FPS_Timer = 0;
	const uint32_t FPS = 1000 / fps;
	int count = 0;

	params.DestFormat = 0;//AV_PIX_FMT_YUV420P
	params.DestWidth = 408;
	params.DestHeight = 720;
	params.UseRenderCallback = false;
	params.UseHadwareDecoder = true;
	params.SyncRender = true;

	CCVideoPlayerAbstract* player = CreateVideoPlayer(&params);
	//player->SetVideoPush(true, "rtsp", "rtsp://127.0.0.1/live");

	//播放器回调
	player->SetPlayerEventCallback([] (CCVideoPlayerAbstract* player, int message, void* eventData, void* customData) {
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

REPLAY:
	player->SetVideoState(CCVideoState::Playing);

	do
	{
		SDL_PollEvent(&playData.event);//轮训方式
		switch (playData.event.type)
		{
		case SDL_QUIT:
			playData.quit = true;
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
		if (player->GetVideoState() == CCVideoState::Ended) {
			count++;
			if (count == 1) {
				player->CloseVideo();
				player->WaitCloseVideo();
				wcscpy_s(strFilename, 260, L"D:\\3.mp4");
				if (!player->OpenVideo(strFilename)) {
					wprintf(L"OpenVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
					goto DESTROY;
				}
				player->WaitOpenVideo();
				goto REPLAY;
			}
			else if (count == 2) {
				player->CloseVideo();
				player->WaitCloseVideo();
				wcscpy_s(strFilename, 260, L"D:\\4.mp4");
				if (!player->OpenVideo(strFilename)) {
					wprintf(L"OpenVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
					goto DESTROY;
				}
				player->WaitOpenVideo();
				goto REPLAY;
			}
			else {
				playData.quit = true;
			}
		}

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
		SDL_DestroyWindow(playData. window);//删除窗口
	
	SDL_Quit();//卸载环境
}

void DoReadVideo(wchar_t* strFilename) {
	auto info = GetVideoInfo(strFilename);

	if (info->success) {
		wprintf(L"GetVideoInfo success\n");
		wprintf(L"duration: %f\n", info->duration);
		wprintf(L"width: %d\n", info->width);
		wprintf(L"height: %d\n", info->height);
		wprintf(L"frameRate: %f\n", info->frameRate);
	}
	else {
		wprintf(L"GetVideoInfo Failed %s\n", info->lastError);
	}

	DoPlayVideo(strFilename, info->width, info->height, (int)info->frameRate);
	ReleaseVideoInfo(info);
}

void ReportFailure(const wchar_t *message) {
	MessageBox(NULL, AppTitle, message, MB_ICONERROR | MB_OK);
}

int MsgThread() {
	int server_sock = 0;
	char buf[SocketBufferSize] = { 0 };

	if (server_sock = nn_socket(AF_SP, NN_PAIR) < 0)
	{
		ReportFailure(L"create server socket failed!");
		goto FAILURE;
	}

	if (nn_bind(server_sock, SocketUrl) < 0)
	{
		ReportFailure(L"bind server sock failed!\r\n");
		goto FAILURE;
	}

	printf("connect socket success\n");

	while (1)
	{
		if (nn_recv(server_sock, buf, sizeof(buf), 0) < 0)
		{
			ReportFailure(L"recv failed!");
			goto FAILURE;
		}
		else
		{
			printf("recieve client msg: %s\r\n", buf);
			if (nn_send(server_sock, buf, sizeof(buf), 0) < 0)
			{
				printf("send failed!\r\n");
				goto FAILURE;
			}
		}
	}

	nn_close(server_sock);
	return 0;
FAILURE:
	if (server_sock)
		nn_close(server_sock);
	exit(EXIT_FAILURE);
	return -1;
}

int main()
{
	setlocale(LC_ALL, "chs");

	std::thread messageThread(MsgThread);
	messageThread.detach();



	return 0;
}
