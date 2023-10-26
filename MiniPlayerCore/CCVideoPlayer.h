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
#include "CCEvent.h"
#include "CCAsyncTaskQueue.h"
#include <thread>
#include <mutex>
#include <condition_variable>

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

    bool SetVideoState(CCVideoState newState);
    bool SetVideoPos(int64_t pos);
    void SetVideoVolume(int vol);
    void SetVideoLoop(bool loop);

    bool GetVideoLoop();
    CCVideoState GetVideoState();
    int64_t GetVideoLength();
    int64_t GetVideoPos();
    int GetVideoVolume();
    void GetVideoSize(int* w, int* h);

    void WaitOpenVideo();
    void WaitCloseVideo();

    //回调
    //**********************

    void SetPlayerEventCallback(CCVideoPlayerEventCallback callback, void* data) {
        videoPlayerEventCallback = callback;
        videoPlayerEventCallbackData = data;
    }
    CCVideoPlayerEventCallback GetPlayerEventCallback() { return videoPlayerEventCallback; }

    //错误处理
    //**********************

    const wchar_t* GetLastErrorMessage();
    const char* GetLastErrorMessageUtf8();
    int GetLastError() const;

    void SetLastError(int code, const wchar_t* errmsg);
    void SetLastError(int code, const char* errmsg);

    void CallPlayerEventCallback(int message, void* data);
    void CallPlayerEventCallback(int message);

    CCVideoPlayerCallbackDeviceData* SyncRenderStart(); 
    void SyncRenderEnd();

    void RenderUpdateDestSize(int width, int height);

    AVPixelFormat GetVideoPixelFormat();

    int PostWorkerThreadCommand(int command, void* data);

protected:

    int lastErrorCode = 0;
    std::wstring lastErrorMessage;
    std::string lastErrorMessageUtf8;
    std::string currentFile;
    bool loop = false;

    CCVideoPlayerEventCallback videoPlayerEventCallback = nullptr;
    void*videoPlayerEventCallbackData = nullptr;

    //解码器相关
    //**********************

    AVFormatContext * formatContext = nullptr;// 解码信息上下文
    const AVCodec * audioCodec = nullptr;// 音频解码器
    const AVCodec * videoCodec = nullptr;// 视频解码器
    AVCodecContext * audioCodecContext = nullptr;//解码器上下文
    AVCodecContext * videoCodecContext = nullptr;//解码器上下文

    AVBufferRef* hw_device_ctx = nullptr;
    enum AVPixelFormat hw_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;
    enum AVPixelFormat hw_frame_pix_fmt = AVPixelFormat::AV_PIX_FMT_NONE;

    static AVPixelFormat GetHwFormat(AVCodecContext* ctx, const AVPixelFormat* pix_fmts);
    int InitHwDecoder(AVCodecContext* ctx, const AVHWDeviceType type);

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

    //异步工作线程
    //**********************

    CCVideoPlayerExternalData externalData;
    CCVideoPlayerInitParams InitParams;

    CCEvent eventWorkerThreadQuit;
    bool workerThreadEnable = false;
    CCAsyncTaskQueue workerQueue;
    std::thread* workerThread;
    void StartWorkerThread();
    void StopWorkerThread();
    static void WorkerThread(CCVideoPlayer* self);
    void PostWorkerCommandFinish(CCAsyncTask* task);

private:
    //线程控制

    void StartDecoderThread();
    void StopDecoderThread();

    bool decoderAudioFinish = false;
    bool decoderVideoFinish = false;

    bool playerSeeking = false;

    std::thread* decoderWorkerThread = nullptr;
    std::thread* decoderAudioThread = nullptr;
    std::thread* decoderVideoThread = nullptr;

    CCEvent decoderAudioThreadDone;
    CCEvent decoderVideoThreadDone;
    CCEvent decoderWorkerThreadDone;

    CCEvent closeDoneEvent;
    CCEvent openDoneEvent;

    static void* DecoderWorkerThreadStub(void*param);
    static void* DecoderVideoThreadStub(void *param);
    static void* DecoderAudioThreadStub(void *param);

    std::mutex decoderWorkerThreadLock;
    std::mutex decoderVideoThreadLock;
    std::mutex decoderAudioThreadLock;

    void* DecoderWorkerThread();
    void* DecoderVideoThread();
    void* DecoderAudioThread();

    //流水线启停

    void StartAll();
    void StopAll();


    //播放器工作子线程控制

    int64_t seekDest = 0;
    int64_t seekPosVideo = 0;
    int64_t seekPosAudio = 0;

    std::mutex setVideoStateLock;


    void DoSetVideoState(CCVideoState state);
    void DoOpenVideo();
    void DoCloseVideo();
    void DoSeekVideo();

    static void FFmpegLogFunc(void* ptr, int level, const char* fmt,va_list vl);
};

#endif //VR720_CCVIDEOPLAYER_H
