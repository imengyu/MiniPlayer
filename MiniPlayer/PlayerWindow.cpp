#include "PlayerWindow.h"
#include "Export.h"
#include <Windows.h>

PlayerWindow::PlayerWindow()
{
}
PlayerWindow::~PlayerWindow()
{
	Destroy();
}

int PlayerWindow::Init()
{	
	//初始化环境
	SDL_Init(0);

	//创建一个窗口
	window = SDL_CreateWindow(
		"PlayVideo",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		viewWidth,
		viewHeight,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
	);
	if (!window)
	{
		LOGEF("SDL_CreateWindow failed: %s", SDL_GetError());
		return -1;
	}

	//创建一个渲染器
	render = SDL_CreateRenderer(window, -1, 0);
	if (!render)
	{
		LOGEF("SDL_CreateRenderer failed: %s", SDL_GetError());
		return -1;
	}

	//启动消息线程
	server.Start();

  return 0;
}
int PlayerWindow::Run()
{
	SDL_Event event;
	do
	{
		SDL_PollEvent(&event);//轮训方式
		switch (event.type)
		{
		case SDL_QUIT:
			quit = true;
			break;
		case SDL_WINDOWEVENT:
			if (event.window.event == SDL_WINDOWEVENT_RESIZED)
				HandleReasize(event.window.data1, event.window.data2);
			break;
		case SDL_MOUSEBUTTONDOWN:
			break;
		default:
			break;
		}

		//帧率限制
		if (SDL_GetTicks() - limitFpsTimer < limitFps)
			SDL_Delay(limitFps - SDL_GetTicks() + limitFpsTimer);
		limitFpsTimer = SDL_GetTicks();

		//渲染
		RenderAllLayout(render);
			
		SDL_RenderPresent(render);//开始渲染图像并拷贝到显示器
	} while (!quit);
	return 0;
}

void PlayerWindow::Destroy()
{
	DeleteAllLayout();

	if (render) {
		SDL_DestroyRenderer(render);//删除渲染器
		render = nullptr;
	}
	if (window) {
		SDL_DestroyWindow(window);//删除窗口
		window = nullptr;
	}

	server.Stop();
	SDL_Quit();//卸载环境
}

void PlayerWindow::HandleReasize(int screenWidth, int screenHeight)
{
	viewWidth = screenWidth;
	viewHeight = screenHeight;
}

void PlayerWindow::RenderAllLayout(SDL_Renderer* render)
{
	for (auto it = layers.begin(); it; it = it->next) {
		auto layer = it->value;
		if (layer->Enable && layer->Opacity > 0) {
			layer->Render(render, viewWidth, viewHeight);
		}
	}
}
void PlayerWindow::DeleteAllLayout()
{
	for (auto it = layers.begin(); it; it = it->next) {
		auto layer = it->value;
		layer->Release();
		delete layer;
	}
	layers.clear();
}

int RunVideoMain(std::string &socketUrl, int socketBufferSize) {
	PlayerWindow window;
	window.GetServer()->SocketUrl = socketUrl;
	window.GetServer()->SocketBufferSize = socketBufferSize;
	if (window.Init() == 0)
		return window.Run();
	return -1;
}

PlayerVideoLayer::PlayerVideoLayer()
{
}
PlayerVideoLayer::~PlayerVideoLayer()
{
}

bool PlayerVideoLayer::Create()
{
	CCVideoPlayerInitParams params;
	params.DestFormat = 0;//AV_PIX_FMT_YUV420P
	params.DestWidth = 720;
	params.DestHeight = 1080;
	params.UseRenderCallback = false;
	params.UseHadwareDecoder = true;
	params.SyncRender = true;

	player = CreateVideoPlayer(&params);
	player->SetPlayerEventCallback([](CCVideoPlayerAbstract* player, int message, void* eventData, void* customData) {
		auto playData = (PlayerVideoLayer*)(customData);
		switch (message)
		{
		case PLAYER_EVENT_CLOSED: {
			break;
		}
		case PLAYER_EVENT_PLAY_END: {
			break;
		}
		case PLAYER_EVENT_INIT_DECODER_DONE: {
			break;
		}
		default:
			break;
		}
	}, this);

	return true;
}
void PlayerVideoLayer::Release()
{
	if (player) {
		if (!player->CloseVideo())
			LOGEF("CloseVideo failed: (%d) %hs", player->GetLastError(), player->GetLastErrorMessage());
		player->WaitCloseVideo();
		DestroyVideoPlayer(player);
		player = nullptr;
	}
}
void PlayerVideoLayer::Render(SDL_Renderer* render, int viewWidth, int viewHeight)
{
	SDL_Rect rect;
	rect.x = X * viewWidth;
	rect.y = Y * viewHeight;
	rect.w = Width * viewWidth;
	rect.h = Height * viewHeight;

	if (player->GetVideoState() == CCVideoState::Playing) {
		auto rdcData = player->SyncRenderStart();
		if (rdcData && rdcData->width > 0 && rdcData->height > 0) {
			SDL_UpdateYUVTexture(
				texture,
				NULL,
				rdcData->data[0], rdcData->linesize[0],
				rdcData->data[1], rdcData->linesize[1],
				rdcData->data[2], rdcData->linesize[2]
			);
		}
		player->SyncRenderEnd();

		SDL_RenderCopy(render, texture, nullptr, &rect);
	}
}
