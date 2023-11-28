#include "Export.h"
#include "PlayerTest.h"
#include <Windows.h>
#include <Commdlg.h>  
#include <Shlobj.h>  
#define SDL_MAIN_HANDLED
#include <SDL.h>

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

	wprintf(L"���֣�%s\n", strFilename);
	if (player->Load(strFilename))
	{

		wprintf(L"�����Ѵ�\n");
		if (player->Play())
		{
			wprintf(L"���ֳ��ȣ�%.3f ��\n", player->GetDuration());
			wprintf(L"���ֿ�ʼ����\n���ڲ��ţ�000.000/%7.3f", player->GetDuration());
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
					wprintf(L"����ʧ�ܣ�%d %s\n", player->GetLastError(), player->GetLastErrorMessage());
					break;
				}

				state = player->GetState();
			}
		}
		else wprintf(L"����ʧ�ܣ�%d %s\n", player->GetLastError(), player->GetLastErrorMessage());

		player->Stop();
		player->Close();
		count++;

		wprintf(L"\n������ֹͣ\n");
	}
	else {
		wprintf(L"���ļ�ʧ�ܣ�%d %s\n", player->GetLastError(), player->GetLastErrorMessage());
	}
	DestroySoundPlayer(player);
}

void DoPlayVideo(wchar_t* strFilename, READ_VIDEO_INFO*info) {
	PlayVideoData playData;
	CCVideoPlayerInitParams params;
	uint32_t _FPS_Timer = 0;
	const uint32_t FPS = 1000 / (info->frameRate > 0 ? info->frameRate : 1);

	params.DestFormat = 0;//AV_PIX_FMT_YUV420P
	params.DestWidth = 408;
	params.DestHeight = 720;
	params.UseRenderCallback = false;
	params.UseHadwareDecoder = true;
	params.SyncRender = true;

	CCVideoPlayerAbstract* player = CreateVideoPlayer(&params);

	//�������ص�
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

	//����Ƶ
	if (!player->OpenVideo(strFilename)) {
		wprintf(L"OpenVideo failed: (%d) %s", player->GetLastError(), player->GetLastErrorMessage());
		goto DESTROY;
	}

	//��ʼ������
	SDL_Init(0);

	//����һ������
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

	//����һ����Ⱦ��
	playData.render = SDL_CreateRenderer(playData.window, -1, 0);
	if (!playData.render)
	{
		wprintf(L"SDL_CreateRenderer failed: %hs", SDL_GetError());
		goto DESTROY;
	}

	//����һ������
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
		SDL_PollEvent(&playData.event);//��ѵ��ʽ
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

		//֡������
		if (SDL_GetTicks() - _FPS_Timer < FPS) {
			SDL_Delay(FPS - SDL_GetTicks() + _FPS_Timer);
		}
		_FPS_Timer = SDL_GetTicks();

		if (info->isVideo) {
			//��Ⱦ
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
				SDL_RenderCopy(playData.render, playData.texture, nullptr, &playData.rect);//���������Կ���Ⱦ
				SDL_RenderPresent(playData.render);//��ʼ��Ⱦͼ�񲢿�������ʾ��
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
		SDL_DestroyTexture(playData.texture);//ɾ������
	if (playData.render)
		SDL_DestroyRenderer(playData.render);//ɾ����Ⱦ��
	if (playData.window)
		SDL_DestroyWindow(playData.window);//ɾ������

	SDL_Quit();//ж�ػ���
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
		ofn.lpstrFilter = TEXT("���ֻ�����Ƶ�ļ�\0;*.mkv;*.mp4;*.mp3;*.wav;*.wma;*.ogg;*.flac;*.aac\0All(*.*)\0*.*\0\0\0");//���ù���
		ofn.nFilterIndex = 1;//����������
		ofn.lpstrFile = strFilename;
		ofn.nMaxFile = sizeof(strFilename);
		ofn.lpstrInitialDir = NULL;
		ofn.lpstrTitle = TEXT("������");
		ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
		if (!GetOpenFileName(&ofn))
		{
			wprintf(L"��ѡ��һ���ļ�\n");
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