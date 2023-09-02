#pragma once
#define WITH_EXPORT_MODE
#include "CSoundPlayer.h"

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
extern "C" CSoundPlayer* __declspec(dllexport) CreatePlayer();

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
extern "C" void __declspec(dllexport) DestroyPlayer(CSoundPlayer* player);

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
extern "C" double __declspec(dllexport) GetAudioDurationFast(const wchar_t* file);

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
extern "C" TStreamFormat __declspec(dllexport) GetAudioFileFormat(const wchar_t* file);