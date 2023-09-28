// MiniPlayer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "Export.h"
#include <Windows.h>
#include <Commdlg.h>  
#include <Shlobj.h>  
#include <locale>
#pragma comment(lib,"Shell32.lib")  
#pragma comment(lib,"MiniPlayerCore.lib")  

CSoundPlayer* player;
CSoundPlayer* player2;

int main()
{
	setlocale(LC_ALL, "chs");
	OPENFILENAME ofn = {0};
	TCHAR strFilename[MAX_PATH] = { 0 };
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetConsoleWindow();
	ofn.lpstrFilter = TEXT("音乐文件\0*.mp4;*.mp3;*.wav;*.mid;*.midi;*.ogg;*.pcm\0All(*.*)\0*.*\0\0\0");//设置过滤  
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
	}

	/*
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

	ReleaseVideoInfo(info);



	*/
	player = CreatePlayer();

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
	DestroyPlayer(player);
	return 0;
}
