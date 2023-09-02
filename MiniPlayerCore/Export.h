#pragma once
#define WITH_EXPORT_MODE
#include "CSoundPlayer.h"

//
// * 说明：
//   创建播放器实例
//
// * 参数：
//   无
// 
// * 返回：
//   播放器实例指针
//
extern "C" CSoundPlayer* __declspec(dllexport) CreatePlayer();

//
// * 说明：
//   销毁播放器实例
//
// * 参数：
//   * player 播放器实例指针
// 
// * 返回：
//   无
//
extern "C" void __declspec(dllexport) DestroyPlayer(CSoundPlayer* player);

//
// * 说明：
//   获取音频文件的时长
//
// * 参数：
//   * file ：文件路径
// 
// * 返回：
//   音频时长（秒）
//
extern "C" double __declspec(dllexport) GetAudioDurationFast(const wchar_t* file);

//
// * 说明：
//   获取音频文件的格式
//
// * 参数：
//   * file ：文件路径
// 
// * 返回：
//   格式枚举
//
extern "C" TStreamFormat __declspec(dllexport) GetAudioFileFormat(const wchar_t* file);