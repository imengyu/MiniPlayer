#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "PlayerServer.h"

class PlayerWindow
{
public:
  PlayerWindow();
  ~PlayerWindow();

  int Init();
  int Run();
  void Destroy();

  PlayerServer* GetServer() { return &server; }

private:

  PlayerServer server;
  SDL_Window* window = nullptr;
  SDL_Renderer* render = nullptr;
};

int RunVideoMain(std::string &socketUrl, int socketBufferSize);

