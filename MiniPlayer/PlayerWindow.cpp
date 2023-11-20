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
		720,
		1080,
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





	return 0;
}

void PlayerWindow::Destroy()
{
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

int RunVideoMain(std::string &socketUrl, int socketBufferSize) {
	PlayerWindow window;
	window.GetServer()->SocketUrl = socketUrl;
	window.GetServer()->SocketBufferSize = socketBufferSize;
	if (window.Init() == 0)
		return window.Run();
	return -1;
}
