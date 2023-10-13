// MiniPlayer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Export.h"
#include <Windows.h>
#include <Commdlg.h>  
#include <Shlobj.h>  
#include <conio.h>  
#include <locale>
#include <mutex>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#pragma comment(lib,"Shell32.lib")  
#pragma comment(lib,"MiniPlayerCore.lib")  
#pragma comment(lib,"SDL2.lib")  
#pragma comment(lib,"SDL2main.lib")

void DoPlaySound(wchar_t* strFilename) {
	CSoundPlayer* player = CreateSoundPlayer();

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

	params.DestFormat = 0;//AV_PIX_FMT_YUV420P
	params.DestWidth = width;
	params.DestHeight = height;
	params.UseRenderCallback = false;
	params.SyncRender = true;

	CCVideoPlayerAbstract* player = CreateVideoPlayer(&params);

	player->SetVideoLoop(true);

	//播放器回调
	player->SetPlayerEventCallback([] (CCVideoPlayerAbstract* player, int message, void* eventData, void* customData) {
		auto playData = (PlayVideoData*)(customData);
		switch (message)
		{
		case PLAYER_EVENT_OPEN_DONE: {
			playData->openSuccess = true;
			player->SetVideoState(CCVideoState::Playing);
			break;
		}
		case PLAYER_EVENT_OPEN_FAIED: {
			playData->openSuccess = false;
			playData->quit = true;
			wprintf(L"OpenVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
			break;
		}
		case PLAYER_EVENT_CLOSED: {
			playData->openSuccess = false;
			break;
		}
		case PLAYER_EVENT_PLAY_DONE: {
			playData->openSuccess = false;
			playData->quit = true;
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
		SDL_WINDOW_SHOWN
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

	do
	{
		SDL_PollEvent(&playData.event);//轮训方式
		switch (playData.event.type)
		{
		case SDL_QUIT:
			playData.quit = true;
			break;
		default:
			break;
		}

		//帧率限制
		if (SDL_GetTicks() - _FPS_Timer < FPS) {
			SDL_Delay(FPS - SDL_GetTicks() + _FPS_Timer);
		}
		_FPS_Timer = SDL_GetTicks();

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

		if (player->GetVideoState() == CCVideoState::Ended)
			playData.quit = true;

	} while (!playData.quit);


DESTROY:
	if (player) {
		if (!player->CloseVideo())
			wprintf(L"CloseVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
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

int endWith(const wchar_t* str1, const wchar_t* str2) {
	if(str1 == NULL || str2 == NULL)
		return -1;
	int len1 = wcslen(str1);
	int len2 = wcslen(str2);
	if ((len1 < len2) || (len1 == 0 || len2 == 0))
		return -1;
	while (len2 >= 1)
	{
		if (str2[len2 - 1] != str1[len1 - 1])
			return 0;
		len2--;
		len1--;
	}
	return 1;
}

int main()
{
	setlocale(LC_ALL, "chs");
	wchar_t strFilename[MAX_PATH] = { 0 };
	wcscpy_s(strFilename, L"D:\\2.mp4");
	/*OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetConsoleWindow();
	ofn.lpstrFilter = TEXT("音乐文件\0*.mp4;*.mp3;*.wav;*.ogg;*.flac;*.aac\0All(*.*)\0*.*\0\0\0");//设置过滤  
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
	}*/

	if (
		endWith(strFilename, L".mp3") == 1 ||
		endWith(strFilename, L".wav") == 1 ||
		endWith(strFilename, L".ogg") == 1 ||
		endWith(strFilename, L".flac") == 1 ||
		endWith(strFilename, L".aac") == 1
	) {
		DoPlaySound(strFilename);
	}
	else {
		DoReadVideo(strFilename);
	}
	
	return 0;
}
