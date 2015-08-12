#pragma once

#ifndef LIBREALSENSE_F200_CAMERA_H
#define LIBREALSENSE_F200_CAMERA_H

#ifndef WIN32
#include "../UVCCamera.h"

namespace f200
{
    
class F200Camera : public rs::UVCCamera
{
    
public:
    
    F200Camera(uvc_device_t * device, int num);
    
    virtual ~F200Camera();
    
    virtual bool ConfigureStreams() override;
    
    virtual void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
    
    virtual void StopStream(int streamIdentifier) override;
};
    
} // end namespace f200
#endif

#endif
