#pragma once

#ifdef MINI_PLAYER_LIB

#include "CSoundPlayer.h"
#include "CCVideoPlayer.h"
#define MINI_PLAYER_EXPORT __declspec(dllexport)

#else

#include "CSoundPlayerExport.h"
#include "CCVideoPlayerExport.h"
#define MINI_PLAYER_EXPORT 
#include "StringHelper.h"

#endif

//
// * ˵����
//   ������Ƶ������ʵ��
//
// * ������
//   param: ��ʼ������
// 
// * ���أ�
//   ������ʵ��ָ��
//
extern "C" MINI_PLAYER_EXPORT CCVideoPlayerAbstract * CreateVideoPlayer(CCVideoPlayerInitParams * param);

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
extern "C" MINI_PLAYER_EXPORT void DestroyVideoPlayer(CCVideoPlayerAbstract * player);

//
// * ˵����
//   ������Ƶ������ʵ��
//
// * ������
//   ��
// 
// * ���أ�
//   ������ʵ��ָ��
//
extern "C" MINI_PLAYER_EXPORT CSoundPlayer * CreateSoundPlayer();

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
extern "C" MINI_PLAYER_EXPORT void DestroySoundPlayer(CSoundPlayer * player);

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

//��ȡ��Ƶ��Ϣ
struct READ_VIDEO_INFO {
  //ָ����ȡ��Ϣ�Ƿ�ɹ�
  bool success;
  //��� success Ϊ false��������ֶ�˵��������Ϣ
  wchar_t lastError[256];
  //��ǰ��Ƶ�ĸ�ʽ��Ϣ
  char format[100];
  //��ǰ��Ƶ��ʱ�����룩
  double duration;
  //��ǰ��Ƶ�Ŀ��
  int width;
  //��ǰ��Ƶ�ĸ߶�
  int height;
  //��ǰ��Ƶ��֡��
  double frameRate;
};

//
// * ˵����
//   �ͷ���Ƶ��ʽ��Ϣ�ṹ��
//
// * ������
//   * ptr ����ʽ��Ϣ�ṹ��
// 
// * ���أ�
//   ��
//
extern "C" MINI_PLAYER_EXPORT void ReleaseVideoInfo(READ_VIDEO_INFO * ptr);

//
// * ˵����
//   ��ȡ��Ƶ�ļ��ĸ�ʽ��Ϣ
//
// * ������
//   * pchFileName ���ļ�·��
// 
// * ���أ�
//   ��ʽ��Ϣ�ṹ�壬����Ҫʹ��ʱ��Ҫ���� ReleaseVideoInfo �����ͷ�
//
extern "C" MINI_PLAYER_EXPORT READ_VIDEO_INFO * GetVideoInfo(const wchar_t* pchFileName);