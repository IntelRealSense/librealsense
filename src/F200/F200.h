#pragma once

#ifndef LIBREALSENSE_F200_CAMERA_H
#define LIBREALSENSE_F200_CAMERA_H

#include "../rs-internal.h"

#ifdef USE_UVC_DEVICES
#include "../UVCCamera.h"
#include "CalibParams.h"

namespace f200
{
    class F200Camera : public rs::UVCCamera
    {
    public:
        F200Camera(uvc_device_t * device, int num);
        ~F200Camera();

        bool ConfigureStreams() override;
        void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
        void StopStream(int streamIdentifier) override;
        RectifiedIntrinsics GetDepthIntrinsics() override { return {}; }
    };
} // end namespace f200
#endif

#endif
