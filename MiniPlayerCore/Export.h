#pragma once

#ifdef MINI_PLAYER_LIB

#include "CSoundPlayer.h"
#define MINI_PLAYER_EXPORT __declspec(dllexport)

#else

#include "CSoundPlayerExport.h"
#define MINI_PLAYER_EXPORT 

#endif

//
// * ˵����
//   ����������ʵ��
//
// * ������
//   ��
// 
// * ���أ�
//   ������ʵ��ָ��
//
extern "C" MINI_PLAYER_EXPORT CSoundPlayer* CreatePlayer();

//
// * ˵����
//   ���ٲ�����ʵ��
//
// * ������
//   * player ������ʵ��ָ��
// 
// * ���أ�
//   ��
//
extern "C" MINI_PLAYER_EXPORT void DestroyPlayer(CSoundPlayer* player);

//
// * ˵����
//   ��ȡ��Ƶ�ļ���ʱ��
//
// * ������
//   * file ���ļ�·��
// 
// * ���أ�
//   ��Ƶʱ�����룩
//
extern "C" MINI_PLAYER_EXPORT double GetAudioDurationFast(const wchar_t* file);

//
// * ˵����
//   ��ȡ��Ƶ�ļ��ĸ�ʽ
//
// * ������
//   * file ���ļ�·��
// 
// * ���أ�
//   ��ʽö��
//
extern "C" MINI_PLAYER_EXPORT TStreamFormat GetAudioFileFormat(const wchar_t* file);