#pragma once

#ifndef LIBREALSENSE_R200_CAMERA_H
#define LIBREALSENSE_R200_CAMERA_H

#include "../UVCCamera.h"

#ifdef USE_UVC_DEVICES
namespace r200
{
    class DS4HardwareIO;

    class R200Camera : public rs::UVCCamera
    {
        std::unique_ptr<DS4HardwareIO> hardware_io;
        rs::StreamConfiguration zConfig;
        rs::StreamConfiguration rgbConfig;
    public:
        R200Camera(uvc_device_t * device, int num);
        ~R200Camera();

        bool ConfigureStreams() override;
        void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
        void StopStream(int streamIdentifier) override;

        rs_intrinsics GetStreamIntrinsics(int stream) override;
        rs_extrinsics GetStreamExtrinsics(int from, int to) override;

        rs::StreamConfiguration GetZConfig() { return zConfig; }
        rs::StreamConfiguration GetRGBConfig() { return rgbConfig; }
    };
} // end namespace r200
#endif

#endif
