#pragma once

#ifndef LIBREALSENSE_R200_CAMERA_H
#define LIBREALSENSE_R200_CAMERA_H

#include <librealsense/CameraContext.h>
#include <librealsense/R200/CameraHeader.h>
#include <librealsense/R200/XU.h>
#include <librealsense/R200/SPI.h>

namespace r200
{

class R200Camera : public rs::UVCCamera
{
    std::unique_ptr<SPI_Interface> spiInterface;
public:
    
    R200Camera(uvc_device_t * device, int num);
    virtual ~R200Camera();
    
    virtual bool ConfigureStreams() override;
    
    virtual void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
    
    virtual void StopStream(int streamIdentifier) override;
    
    RectifiedIntrinsics GetRectifiedIntrinsicsZ();
};
    
} // end namespace r200

#endif