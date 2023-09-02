#pragma once
#include "CSoundPlayer.h"
#define WITH_EXPORT_MODE

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
extern "C" __declspec(dllexport) CSoundPlayer * CreatePlayer();

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
extern "C" __declspec(dllexport) void DestroyPlayer(CSoundPlayer* player);