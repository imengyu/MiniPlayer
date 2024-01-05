// MiniPlayer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Export.h"
#include "PlayerTest.h"
#include "PlayerWindow.h"
#include "argparse.hpp"
#include <conio.h>  
#include <locale>

const char* SocketUrl = "tcp://127.0.0.1:4173";
const int SocketBufferSize = 256;
const char* VERSION = "1.0.0";

int main(int argc, const char* argv[])
{
	setlocale(LC_ALL, "chs");
  return TestPCMToWav();

  /*
  if (argc > 1) {
    auto args = util::argparser("A standalone video player program.");
    args.set_program_name("MiniPlayer")
      .add_help_option()
      .add_sc_option("-v", "--version", "show version info", []() {
        std::cout << "version " << VERSION << std::endl;
      })
      .add_option("-s", "--server", "run as server, use socket")
      .add_option<std::string>("", "--socket-address", "set the socket address", std::string(SocketUrl))
      .add_option<int>("", "--socket-buf-size", "set the socket buffer size", (int)SocketBufferSize)
      .add_argument<std::string>("input", "play a video or audio file (without --server)")
      .parse(argc, argv);

    if (args.has_option("--server")) {

      std::string socketUrl = SocketUrl;
      int socketBufferSize = SocketBufferSize;

      if (args.has_option("--socket-address"))
        socketUrl = args.get_option_string("--socket-address");
      if (args.has_option("--socket-buf-size"))
        socketBufferSize = args.get_argument_int("--socket-buf-size");

      return RunVideoMain(socketUrl, socketBufferSize);
    }

    auto path = args.get_argument_string_raw("input");
    return RunPlayer(path.c_str());
  }
	return RunPlayer(nullptr);
  */
}
