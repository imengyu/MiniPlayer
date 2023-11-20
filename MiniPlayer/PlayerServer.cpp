#include "PlayerServer.h"

PlayerServer::~PlayerServer()
{
	Stop();
}

void PlayerServer::Start() {
	serverQuit = false;
	std::thread msgThread(MsgThreadStub, this);
	msgThread.detach();
}

void PlayerServer::Stop() {
	serverQuitEvent.Reset();
	serverQuitEvent.Wait();
	serverQuit = true;
}

int PlayerServer::MsgThreadStub(PlayerServer* ptr)
{
	int ret = ptr->MsgThread();
	ptr->serverQuitEvent.NotifyOne();
	return ret;
}

int PlayerServer::MsgThread() {
	int server_sock = 0;
	char* buf = new char[SocketBufferSize];

	if (server_sock = nn_socket(AF_SP, NN_PAIR) < 0)
	{
		LOGE("Create server socket failed!");
		goto FAILURE;
	}

	if (nn_bind(server_sock, SocketUrl.c_str()) < 0)
	{
		LOGE("Bind server sock failed!");
		goto FAILURE;
	}

	LOGD("Connect socket success");

	LOGDF("Server listen on socket: %s", SocketUrl);

	while (!serverQuit)
	{
		if (nn_recv(server_sock, buf, sizeof(buf), 0) < 0)
		{
			LOGE("Recv failed!");
			goto FAILURE;
		}
		else
		{
			printf("Recieve client msg: %s", buf);
			if (nn_send(server_sock, buf, sizeof(buf), 0) < 0)
			{
				LOGE("Send failed!");
				goto FAILURE;
			}
		}
	}

	nn_close(server_sock);
	return 0;
FAILURE:
	if (server_sock)
		nn_close(server_sock);
	return -1;
}
