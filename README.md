# MiniPlayer

MiniPlayer 是一个 Windows 平台基于 WASAPI 和 ffmpeg 的音视频播放库。

## 使用

1. 编译库
2. 引用 Export.h
3. 创建播放器并打开音乐播放：

  ```cpp
  int main() {
    CSoundPlayer* player = CreateSoundPlayer();
    if (player->Load(strFilename))
    {
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
    }
    else {
      wprintf(L"打开文件失败：%d %s\n", player->GetLastError(), player->GetLastErrorMessage());
    }
    DestroySoundPlayer(player);
    return 0;
  }
  ```

## 其中使用了以下一些开源解码库

* libmpg123
* libogg
* libvorbis
* libFLAC
* libffad
* libffac
