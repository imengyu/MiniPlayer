//
// Created by roger on 2020/12/22.
//

#ifndef VR720_CCPLAYERRENDER_H
#define VR720_CCPLAYERRENDER_H
#include "pch.h"
#include <thread>
extern "C" {
#include "libavutil/frame.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}
#include "CCPlayerDefine.h"
#include "CSoundDevice.h"

class CCVideoDevice;
class CCVideoPlayerExternalData;
class CCPlayerRender : public CSoundDeviceHoster {
public:
    virtual bool Init(CCVideoPlayerExternalData *data);
    virtual void Destroy();
    virtual void Start(bool isStartBySeek);
    virtual void Reset();
    virtual void Stop();

    virtual CCVideoDevice* GetVideoDevice() { return videoDevice; }
    virtual CSoundDevice* GetAudioDevice() { return audioDevice; }

    virtual int64_t GetCurVideoDts() { return curVideoDts; }
    virtual int64_t GetCurVideoPts() { return curVideoPts; }
    virtual int64_t GetCurAudioDts() { return curAudioDts; }
    virtual int64_t GetCurAudioPts() { return curAudioPts; }

    virtual void SetVolume(int i);
    virtual int GetVolume() { return 0; }

    virtual bool GetCurrentSeekToPosFinished() {
        if (currentSeekToPosFinished) {
            currentSeekToPosFinished = false;
            return true;
        }
        return false;
    }

    bool GetShouldReSample() { return false; }
    void SetLastError(int code, const wchar_t* errmsg);
protected:

    virtual CCVideoDevice* CreateVideoDevice();
    virtual CSoundDevice*CreateAudioDevice(CCVideoPlayerExternalData *data);

private:

    SwrContext *swrContext = NULL;
    SwsContext *swsContext = nullptr;

    int64_t curVideoDts = 0;	//记录当前播放的视频流Packet的DTS
    int64_t curVideoPts = 0;	//记录当前播放的视频流Packet的DTS
    int64_t curAudioDts = 0;	//记录当前播放的音频流Packet的DTS
    int64_t curAudioPts = 0;	//记录当前播放的音频流Packet的DTS

    // 输出缓冲
    uint8_t *audioOutBuffer[1] = { nullptr };
    // 重采样后，每个通道包含的采样数
    // acc默认为1024，重采样后可能会变化
    int destNbSample = 1024;
    // 重采样以后，一帧数据的大小
    size_t destDataSize = 0;

    //时钟
    double currentAudioClock = 0;
    double currentVideoClock = 0;


    //***************************************
    //状态控制

    CCVideoPlayerExternalData *externalData;

    CCRenderState status = CCRenderState::NotRender;
    bool currentSeekToPosFinished = false;

    CSoundDevice* audioDevice = nullptr;
    CCVideoDevice* videoDevice = nullptr;

    AVFrame *outFrame = nullptr;
    uint8_t *outFrameBuffer = nullptr;
    size_t outFrameBufferSize = 0;
    int outFrameDestWidth = 0;
    int outFrameDestHeight = 0;
    AVPixelFormat outFrameDestFormat = AV_PIX_FMT_RGBA;

    std::thread* renderVideoThread = nullptr;
    std::thread* renderAudioThread = nullptr;

    static void* RenderVideoThreadStub(void *param);
    void* RenderVideoThread();

    static bool RenderAudioBufferDataStub(CSoundDeviceHoster* instance, LPVOID buf, DWORD buf_len);
    bool RenderAudioBufferData(uint8_t** buf, DWORD* len);
};


#endif //VR720_CCPLAYERRENDER_H
