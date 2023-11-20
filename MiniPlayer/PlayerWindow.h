#pragma once
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <vector>
#include "PlayerServer.h"
#include "CCSimpleQueue.h"

class PlayerLayer;
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

  bool quit = false;
  int limitFps = 1000 / 25;
  uint32_t limitFpsTimer = 0;

  int viewWidth = 720;
  int viewHeight = 1080;

  PlayerServer server;
  CCSimpleQueue<PlayerLayer> layers;
  SDL_Window* window = nullptr;
  SDL_Renderer* render = nullptr;

  void HandleReasize(int screenWidth, int screenHeight);
  void RenderAllLayout(SDL_Renderer* render);
  void DeleteAllLayout();
};

class PlayerLayer {
public:
  int Id = 0;
  float X = 0;
  float Y = 0;
  float Width = 0;
  float Height = 0;
  float Rotate = 0;
  float Opacity = 1;
  bool Enable = true;

  virtual void Render(SDL_Renderer* render, int viewWidth, int viewHeight) {}
  virtual bool Create() {}
  virtual void Release() {}
};

class PlayerVideoLayer : public PlayerLayer {

public:
  PlayerVideoLayer();
  ~PlayerVideoLayer();

  bool Create();
  void Release();
  void Render(SDL_Renderer* renders, int viewWidth, int viewHeight);
private:

  SDL_Texture* texture = nullptr;
  CCVideoPlayerAbstract* player = nullptr;
};

int RunVideoMain(std::string &socketUrl, int socketBufferSize);

