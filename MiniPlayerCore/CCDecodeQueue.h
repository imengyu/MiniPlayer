﻿//
// Created by roger on 2020/12/21.
//

#ifndef VR720_CCDECODEQUEUE_H
#define VR720_CCDECODEQUEUE_H

#include <thread>
#include <mutex>
#include <list>
#include "pch.h"
extern "C" {
#include "libavcodec/avcodec.h"
}

/**
 * 解码队列控制类
 */
class CCVideoPlayerExternalData;
class CCDecodeQueue {

public:

    void Init(CCVideoPlayerExternalData *data);
    void Reset();
    void Destroy();

    void AllocPacketPool(int size);
    void ReleasePacketPool();
    void ReleasePacket(AVPacket *pkt);
    AVPacket* RequestPacket();

    void ReleaseFramePool();
    void AllocFramePool(int size);
    AVFrame *RequestFrame();
    void ReleaseFrame(AVFrame *frame);

    size_t AudioQueueSize();
    void AudioEnqueue(AVPacket *pkt);
    AVPacket* AudioDequeue();
    void AudioQueueBack(AVPacket *packet);

    int VideoDrop(double targetClock);
    size_t VideoQueueSize();
    void VideoEnqueue(AVPacket *pkt);
    AVPacket* VideoDequeue();
    void VideoQueueBack(AVPacket *packet);

    void AudioFrameEnqueue(AVFrame *frame);
    AVFrame* AudioFrameDequeue();

    void VideoFrameEnqueue(AVFrame *frame);
    AVFrame* VideoFrameDequeue();

    size_t VideoFrameQueueSize();
    size_t AudioFrameQueueSize();

    void ClearAll();
private:
    std::list<AVPacket*> packetPool;
    std::list<AVFrame*> framePool;
    std::list<AVPacket*> videoQueue;
    std::list<AVPacket*> audioQueue;
    std::list<AVFrame*>  videoFrameQueue;
    std::list<AVFrame*>  audioFrameQueue;

    bool initState = false;
    CCVideoPlayerExternalData * externalData;

    std::mutex packetRequestLock;
    std::mutex frameRequestLock;
};


#endif //VR720_CCDECODEQUEUE_H
