#pragma once

#ifndef LIBREALSENSE_R200_CAMERA_H
#define LIBREALSENSE_R200_CAMERA_H

#include <librealsense/CameraContext.h>

#include <librealsense/R200/R200_XU.h>
#include <librealsense/R200/R200_SPI.h>
#include <librealsense/R200/R200_CameraHeader.h>

enum Imager
{
    IMAGER_BOTH,
    IMAGER_THIRD
};

class R200Camera : public UVCCamera
{
    
    std::unique_ptr<SPI_Interface> spiInterface;
    
public:
    
    R200Camera(uvc_device_t * device, int num);
    virtual ~R200Camera();
    
    virtual bool ConfigureStreams() override;
    
    virtual void StartStream(int streamIdentifier, const StreamConfiguration & config) override;
    
    virtual void StopStream(int streamIdentifier) override;
    
    // @tofix get rid of this function
    // void DumpInfo();
    
    RectifiedIntrinsics GetCalibrationDataRectZ()
    {
        // Check if SPI interface even exists.
        return spiInterface->GetZIntrinsics();
    }
    
};

#endif