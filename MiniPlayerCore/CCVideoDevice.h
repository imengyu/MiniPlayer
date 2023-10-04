//
// Created by roger on 2020/12/22.
//

#ifndef VR720_CCVIDEODEVICE_H
#define VR720_CCVIDEODEVICE_H

#include "pch.h"
extern "C" {
#include "libavutil/frame.h"
}

class CCVideoDevice {

public:
    CCVideoDevice() {}
    virtual ~CCVideoDevice() {}

    virtual void Destroy() {}
    virtual uint8_t* Lock(uint8_t *src, int srcStride, int*destStride, int64_t pts) { return nullptr; }
    virtual void Unlock() {}
    virtual void Dirty() {}

    virtual void Pause() {}
    virtual void Reusme() {}
    virtual void Reset() {}
};


#endif //VR720_CCVIDEODEVICE_H
