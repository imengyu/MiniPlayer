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
    virtual void Render(AVFrame *frame, int64_t pts) {}

    virtual void Pause() {}
    virtual void Reusme() {}
    virtual void Reset() {}
};


#endif //VR720_CCVIDEODEVICE_H
