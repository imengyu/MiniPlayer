#pragma once
#include "CSoundPlayer.h"
#define WITH_EXPORT_MODE

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
extern "C" __declspec(dllexport) CSoundPlayer * CreatePlayer();

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
extern "C" __declspec(dllexport) void DestroyPlayer(CSoundPlayer* player);