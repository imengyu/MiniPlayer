//
// Created by roger on 2020/12/21.
//

#include "pch.h"
#include "CCDecodeQueue.h"
#include "CCVideoPlayer.h"
#include "Logger.h"

void CCDecodeQueue::Init(CCVideoPlayerExternalData* data) {

  if (!initState) {
    initState = true;

    packetPool.alloc(data->InitParams->PacketPoolSize);
    framePool.alloc(data->InitParams->FramePoolSize);
    videoQueue.alloc(data->InitParams->FramePoolSize);
    audioQueue.alloc(data->InitParams->FramePoolSize);
    videoFrameQueue.alloc(data->InitParams->FramePoolSize);
    audioFrameQueue.alloc(data->InitParams->FramePoolSize);

    AllocFramePool(data->InitParams->FramePoolSize);
    AllocPacketPool(data->InitParams->PacketPoolSize);

    externalData = data;
  }
}
void CCDecodeQueue::Reset() {

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

  audioQueue.push(pkt);

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

  audioQueue.push(packet);

  audioFrameQueueRequestLock.unlock();
}
size_t CCDecodeQueue::AudioQueueSize() {
  return audioQueue.size();
}
void CCDecodeQueue::VideoEnqueue(AVPacket* pkt) {
  videoFrameQueueRequestLock.lock();

  videoQueue.push(pkt);

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

  videoQueue.push_front(packet);

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

  videoFrameQueue.push(frame);

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

  audioFrameQueue.push(frame);

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

  if (pkt)
    av_packet_unref(pkt);
  if (!initState)
    return;

  packetRequestLock.lock();

  if (packetPool.size() >= (int)externalData->InitParams->PacketPoolSize) {
    av_packet_free(&pkt);
  }
  else {

    packetPool.push(pkt);
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
    return nullptr;
  }

  AVPacket* packet = packetPool.pop_front();

  packetRequestLock.unlock();

  return packet;
}
void CCDecodeQueue::AllocPacketPool(int size) {
  for (int i = 0; i < size; i++)
    if (!packetPool.push(av_packet_alloc())) {
      LOGEF("Packet Pool size too large! Size: %d", packetPool.size());
      break;
    }
}
void CCDecodeQueue::ReleasePacketPool() {
  for (auto packet = packetPool.begin(); packet; packet = packet->next)
    av_packet_free(&packet->value);
  packetPool.clear();
}

void CCDecodeQueue::ReleaseFrame(AVFrame* frame) {

  if (!initState)
    return;

  frameRequestLock.lock();

  if (framePool.size() >= (int)externalData->InitParams->FramePoolSize) {
    av_frame_free(&frame);
  }
  else {
    av_frame_unref(frame);
    framePool.push(frame);
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
    return nullptr;
  }

  AVFrame* frame = framePool.pop_front();

  frameRequestLock.unlock();

  return frame;
}
void CCDecodeQueue::AllocFramePool(int size) {
  for (int i = 0; i < size; i++)
    if (!framePool.push(av_frame_alloc())) {
      LOGEF("Frame Pool size too large! Size: %d", framePool.size());
      break;
    }
}
void CCDecodeQueue::ReleaseFramePool() {
  for (auto frame = framePool.begin(); frame; frame = frame->next)
    av_frame_free(&frame->value);
  framePool.clear();
}