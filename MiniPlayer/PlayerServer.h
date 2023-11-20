#pragma once
#include "Export.h"
#include <thread>
#include <mutex>
#include <nanomsg/pair.h>
#include <nanomsg/bus.h>
#include <nanomsg/nn.h>


class PlayerServer
{
public:
  ~PlayerServer();

  void Start();
  void Stop();

  std::string SocketUrl;
  int SocketBufferSize;
private:
  bool serverQuit = false;
  CCEvent serverQuitEvent;

  static int MsgThreadStub(PlayerServer* ptr);
  int MsgThread();
};

