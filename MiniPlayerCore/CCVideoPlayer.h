//
// Created by roger on 2020/12/19.
//

#ifndef VR720_CCVIDEOPLAYER_H
#define VR720_CCVIDEOPLAYER_H
#include "pch.h"
#include "CCVideoPlayerExport.h"
#include "CCPlayerDefine.h"
#include "CCDecodeQueue.h"
#include "CCPlayerRender.h"
#include <thread>
#include <mutex>


/**
 * 简单视频播放器
 */
class CCVideoPlayer : public CCVideoPlayerAbstract {

public:
    CCVideoPlayer();
    CCVideoPlayer(CCVideoPlayerInitParams * initParams);
    ~CCVideoPlayer();

    static void GlobalInit();

    //初始配置和状态信息
    //**********************

    virtual CCVideoPlayerInitParams* GetInitParams() { return &InitParams; }

    //播放器公共方法
    //**********************

    bool OpenVideo(const char* filePath);
    bool OpenVideo(const wchar_t* filePath);
    bool CloseVideo();

    void SetVideoState(CCVideoState newState);
    void SetVideoPos(int64_t pos);
    void SetVideoVolume(int vol);

    CCVideoState GetVideoState();
    int64_t GetVideoLength();
    int64_t GetVideoPos();
    int GetVideoVolume();
    void GetVideoSize(int* w, int* h);

    //回调
    //**********************

    void SetPlayerEventCallback(CCVideoPlayerEventCallback callback, void* data) {
        videoPlayerEventCallback = callback;
        videoPlayerEventCallbackData = data;
    }
    CCVideoPlayerEventCallback GetPlayerEventCallback() { return videoPlayerEventCallback; }

    //错误处理
    //**********************

    void SetLastError(int code, const wchar_t* errmsg);
    void SetLastError(int code, const char* errmsg);
    const wchar_t* GetLastErrorMessage();
    int GetLastError() const;

    void CallPlayerEventCallback(int message, void* data);
    void CallPlayerEventCallback(int message);

    void SyncRender();
protected:

    int lastErrorCode = 0;
    std::wstring lastErrorMessage;
    std::string currentFile;

    CCVideoPlayerEventCallback videoPlayerEventCallback = nullptr;
    void*videoPlayerEventCallbackData = nullptr;

    //解码器相关
    //**********************

    AVFormatContext * formatContext = nullptr;// 解码信息上下文
    const AVCodec * audioCodec = nullptr;// 音频解码器
    const AVCodec * videoCodec = nullptr;// 视频解码器
    AVCodecContext * audioCodecContext = nullptr;//解码器上下文
    AVCodecContext * videoCodecContext = nullptr;//解码器上下文

    long duration = 0;// 总时长

    CCDecodeState decodeState = CCDecodeState::NotInit;// 解码状态
    CCVideoState videoState = CCVideoState::NotOpen;

    CCDecodeQueue decodeQueue;
    CCPlayerRender *render = nullptr;

    int videoIndex = -1;// 数据流索引
    int audioIndex = -1;// 数据流索引

    bool InitDecoder();
    bool DestroyDecoder();

    void Init(CCVideoPlayerInitParams *initParams);
    void Destroy();

    CCVideoPlayerExternalData externalData;
    CCVideoPlayerInitParams InitParams;

private:
    //线程控制

    void StartDecoderThread(bool isStartBySeek = false);
    void StopDecoderThread();

    bool playerWorking = false;
    bool decoderAudioFinish = false;
    bool decoderVideoFinish = false;

    std::thread* decoderWorkerThread = nullptr;
    std::thread* decoderAudioThread = nullptr;
    std::thread* decoderVideoThread = nullptr;
    std::thread* playerWorkerThread = nullptr;

    static void* PlayerWorkerThreadStub(void*param);
    static void* DecoderWorkerThreadStub(void*param);
    static void* DecoderVideoThreadStub(void *param);
    static void* DecoderAudioThreadStub(void *param);

    void* PlayerWorkerThread();
    void* DecoderWorkerThread();
    void* DecoderVideoThread();
    void* DecoderAudioThread();

    //流水线启停

    std::mutex startAllLock;
    std::mutex stopAllLock;

    void StartAll();
    void StopAll();

    //播放器工作子线程控制

    UCHAR playerClose = 0;
    UCHAR playerOpen = 0;
    UCHAR playerSeeking = 0;

    int64_t seekDest = 0;
    int64_t seekPosVideo = 0;
    int64_t seekPosAudio = 0;

    void DoOpenVideo();
    void DoCloseVideo();
    void DoSeekVideo();

    static void FFmpegLogFunc(void* ptr, int level, const char* fmt,va_list vl);
};

#endif //VR720_CCVIDEOPLAYER_H
