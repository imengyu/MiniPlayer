//
// Created by roger on 2020/12/21.
//

#include "pch.h"
#include "DbgHelper.h"
#include "CCDecodeQueue.h"
#include "CCVideoPlayer.h"
#include "Logger.h"

#define MAX_INCREASE_VIDEO_QUEUE_SIZE 1024
#define MAX_INCREASE_AUIDO_QUEUE_SIZE MAX_INCREASE_VIDEO_QUEUE_SIZE * 8
#define MAX_ALLOCATE_FRAME 1300

void CCDecodeQueue::Init(CCVideoPlayerExternalData* data) {

  if (!initState) {
    initState = true;

    packetPool.alloc(data->InitParams->PacketPoolSize);
    framePool.alloc(data->InitParams->FramePoolSize);

    videoQueue.alloc(data->InitParams->MaxRenderQueueSize * 2);
    audioQueue.alloc(data->InitParams->MaxRenderQueueSize * 8);
    videoFrameQueue.alloc(data->InitParams->MaxRenderQueueSize * 2);
    audioFrameQueue.alloc(data->InitParams->MaxRenderQueueSize * 8);

    AllocFramePool(data->InitParams->FramePoolSize);
    AllocPacketPool(data->InitParams->PacketPoolSize);

    externalData = data;
  }
}
void CCDecodeQueue::Reset() {
  if (initState)
    ClearAll();
}
void CCDecodeQueue::Destroy() {

  if (initState) {
    initState = false;
    ClearAll();
    ReleasePacketPool();
    ReleaseFramePool();
  }
}

//包数据队列

void CCDecodeQueue::AudioEnqueue(AVPacket* pkt) {
  audioFrameQueueRequestLock.lock();

  if (!audioQueue.push(pkt)) {
    audioQueue.increase(externalData->InitParams->RenderQueueSizeGrowStep);
    audioQueue.push(pkt);
  }

  Assert(audioQueue.capacity() < MAX_INCREASE_AUIDO_QUEUE_SIZE);

  audioFrameQueueRequestLock.unlock();
}
AVPacket* CCDecodeQueue::AudioDequeue() {
  audioFrameQueueRequestLock.lock();
  if (audioQueue.empty()) {

    audioFrameQueueRequestLock.unlock();
    return nullptr;
  }
  AVPacket* pkt = audioQueue.pop_front();

  audioFrameQueueRequestLock.unlock();

  return pkt;
}
void CCDecodeQueue::AudioQueueBack(AVPacket* packet) {
  audioFrameQueueRequestLock.lock();

  Assert(audioQueue.push(packet));

  audioFrameQueueRequestLock.unlock();
}
size_t CCDecodeQueue::AudioQueueSize() {
  return audioQueue.size();
}
void CCDecodeQueue::VideoEnqueue(AVPacket* pkt) {
  videoFrameQueueRequestLock.lock();

  if (!videoQueue.push(pkt)) {
    videoQueue.increase(externalData->InitParams->RenderQueueSizeGrowStep);
    videoQueue.push(pkt);
  }

  Assert(videoQueue.capacity() < MAX_INCREASE_VIDEO_QUEUE_SIZE);

  videoFrameQueueRequestLock.unlock();
}
AVPacket* CCDecodeQueue::VideoDequeue() {
  videoFrameQueueRequestLock.lock();

  if (videoQueue.empty()) {
    videoFrameQueueRequestLock.unlock();
    return nullptr;
  }

  AVPacket* pkt = videoQueue.pop_front();

  videoFrameQueueRequestLock.unlock();

  return pkt;
}
void CCDecodeQueue::VideoQueueBack(AVPacket* packet) {
  videoFrameQueueRequestLock.lock();

  Assert(videoQueue.push_front(packet));

  videoFrameQueueRequestLock.unlock();
}
size_t CCDecodeQueue::VideoQueueSize() {
  return videoQueue.size();
}

//已经解码的数据队列

AVFrame* CCDecodeQueue::VideoFrameDequeue() {

  videoFrameQueueRequestLock.lock();

  if (videoFrameQueue.empty()) {
    videoFrameQueueRequestLock.unlock();
    return nullptr;
  }

  AVFrame* frame = videoFrameQueue.pop_front();

  videoFrameQueueRequestLock.unlock();

  return frame;
}
void CCDecodeQueue::VideoFrameEnqueue(AVFrame* frame) {
  videoFrameQueueRequestLock.lock();

  Assert(videoFrameQueue.push(frame));

  videoFrameQueueRequestLock.unlock();
}
AVFrame* CCDecodeQueue::AudioFrameDequeue() {

  audioFrameQueueRequestLock.lock();

  if (audioFrameQueue.empty()) {
    audioFrameQueueRequestLock.unlock();
    return nullptr;
  }

  AVFrame* frame = audioFrameQueue.pop_front();

  audioFrameQueueRequestLock.unlock();
  return frame;
}
void CCDecodeQueue::AudioFrameEnqueue(AVFrame* frame) {
  audioFrameQueueRequestLock.lock();

  Assert(audioFrameQueue.push(frame));

  audioFrameQueueRequestLock.unlock();
}

size_t CCDecodeQueue::VideoFrameQueueSize() {
  return videoFrameQueue.size();
}
size_t CCDecodeQueue::AudioFrameQueueSize() {
  return audioFrameQueue.size();
}

//清空队列数据
void CCDecodeQueue::ClearAll() {
  videoFrameQueueRequestLock.lock();
  audioFrameQueueRequestLock.lock();

  for (auto frame = videoFrameQueue.begin(); frame; frame = frame->next)
    ReleaseFrame(frame->value);
  for (auto frame = audioFrameQueue.begin(); frame; frame = frame->next)
    ReleaseFrame(frame->value);
  for (auto packet = videoQueue.begin(); packet; packet = packet->next)
    ReleasePacket(packet->value);
  for (auto packet = audioQueue.begin(); packet; packet = packet->next)
    ReleasePacket(packet->value);

  videoFrameQueue.clear();
  audioFrameQueue.clear();
  videoQueue.clear();
  audioQueue.clear();

  videoFrameQueueRequestLock.unlock();
  audioFrameQueueRequestLock.unlock();
}

//视频太慢丢包
int CCDecodeQueue::VideoDrop(double targetClock) {

  double frameClock;
  int droppedCount = 0;
  if (!videoFrameQueue.empty()) {
    for (auto it = videoFrameQueue.begin(); it;) {
      AVFrame* frame = it->value;
      frameClock = frame->best_effort_timestamp == AV_NOPTS_VALUE ?
        frame->pts : frame->best_effort_timestamp * av_q2d(externalData->VideoTimeBase);
      if ((droppedCount >= (int)videoFrameQueue.size() / 3
        || frameClock >= targetClock)
        && it != videoFrameQueue.begin())
        break;
      else {
        ReleaseFrame(frame);
        it = videoFrameQueue.erase(it);
        droppedCount++;
      }
    }
  }

  return droppedCount;
}

//两个池
//可以随用随取，防止重复申请释放内存的额外开销

void CCDecodeQueue::ReleasePacket(AVPacket* pkt) {
  packetRequestLock.lock();

  av_packet_unref(pkt);
  if (!packetPool.push(pkt)) {
    av_packet_free(&pkt);
    allocedPacket--;
  }

  packetRequestLock.unlock();
}
AVPacket* CCDecodeQueue::RequestPacket() {

  if (!initState)
    return nullptr;

  packetRequestLock.lock();

  if (packetPool.empty()) {
    packetPool.increase(externalData->InitParams->PacketPoolGrowStep);
    AllocPacketPool(externalData->InitParams->PacketPoolGrowStep);
  }
  if (packetPool.empty()) {
    LOGEF("Failed to increase packet pool!");
    packetRequestLock.unlock();
    return nullptr;
  }

  AVPacket* packet = packetPool.pop_front();

  packetRequestLock.unlock();

  return packet;
}
void CCDecodeQueue::AllocPacketPool(int size) {
  for (int i = 0; i < size; i++) {
    allocedPacket++;
    if (!packetPool.push(av_packet_alloc())) {
      LOGEF("Packet Pool size too large! Size: %d", packetPool.size());
      break;
    }
    Assert(allocedPacket < MAX_ALLOCATE_FRAME);
  }
}
void CCDecodeQueue::ClearPacketPool() {
  int freePacket = 0;
  for (auto packet = packetPool.begin(); packet; packet = packet->next) {
    av_packet_free(&packet->value);
    freePacket++;
  }

  Assert(freePacket == allocedPacket);
  allocedPacket = 0;

  packetPool.clear();
}
void CCDecodeQueue::ReleasePacketPool() {
  ClearPacketPool();
  packetPool.release();
}

void CCDecodeQueue::ReleaseFrame(AVFrame* frame) {
  frameRequestLock.lock();

  av_frame_unref(frame);

  if (!framePool.push(frame)) {
    av_frame_free(&frame);
    allocedFrame--;
  }

  frameRequestLock.unlock();
}
AVFrame* CCDecodeQueue::RequestFrame() {

  if (!initState)
    return nullptr;

  frameRequestLock.lock();

  if (framePool.empty()) {
    framePool.increase(externalData->InitParams->FramePoolGrowStep);
    AllocFramePool(externalData->InitParams->FramePoolGrowStep);
  }
  if (framePool.empty()) {
    LOGE("Failed to increase frame pool!");
    frameRequestLock.unlock();
    return nullptr;
  }

  AVFrame* frame = framePool.pop_front();

  frameRequestLock.unlock();

  return frame;
}
void CCDecodeQueue::AllocFramePool(int size) {
  for (int i = 0; i < size; i++) {
    allocedFrame++;
    if (!framePool.push(av_frame_alloc())) {
      LOGEF("Frame Pool size too large! Size: %d", framePool.size());
      break;
    }
    Assert(allocedFrame < MAX_ALLOCATE_FRAME);
  }
}
void CCDecodeQueue::ClearFramePool() {
  int freeFrame = 0;
  for (auto frame = framePool.begin(); frame; frame = frame->next) {
    av_frame_free(&frame->value);
    freeFrame++;
  }

  Assert(freeFrame == allocedFrame);
  allocedFrame = 0;
}
void CCDecodeQueue::ReleaseFramePool() {
  ClearFramePool();
  framePool.release();
}