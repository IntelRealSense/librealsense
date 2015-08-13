#pragma once

#ifndef LIBREALSENSE_F200_CAMERA_H
#define LIBREALSENSE_F200_CAMERA_H

#include "../rs-internal.h"

#ifdef USE_UVC_DEVICES
#include "../UVCCamera.h"
#include "F200Types.h"
#include "HardwareIO.h"

namespace f200
{
    
class F200Camera : public rs::UVCCamera
{
    
    std::unique_ptr<IVCAMHardwareIO> hardware_io;
    
public:
    
    F200Camera(uvc_context_t * ctx, uvc_device_t * device, int num);
    ~F200Camera();
    
    bool ConfigureStreams() override;
    void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
    void StopStream(int streamIdentifier) override;

    rs_intrinsics GetStreamIntrinsics(int stream) override;
    rs_extrinsics GetStreamExtrinsics(int from, int to) override;
};

} // end namespace f200
#endif

#endif
