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
	//��ʼ������
	SDL_Init(0);

	//����һ������
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

	//����һ����Ⱦ��
	render = SDL_CreateRenderer(window, -1, 0);
	if (!render)
	{
		LOGEF("SDL_CreateRenderer failed: %s", SDL_GetError());
		return -1;
	}

	//������Ϣ�߳�
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
		SDL_DestroyRenderer(render);//ɾ����Ⱦ��
		render = nullptr;
	}
	if (window) {
		SDL_DestroyWindow(window);//ɾ������
		window = nullptr;
	}

	server.Stop();
	SDL_Quit();//ж�ػ���
}

int RunVideoMain(std::string &socketUrl, int socketBufferSize) {
	PlayerWindow window;
	window.GetServer()->SocketUrl = socketUrl;
	window.GetServer()->SocketBufferSize = socketBufferSize;
	if (window.Init() == 0)
		return window.Run();
	return -1;
}
