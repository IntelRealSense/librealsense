#pragma once

#ifndef LIBREALSENSE_R200_CAMERA_H
#define LIBREALSENSE_R200_CAMERA_H

#include "../UVCCamera.h"
#include "HardwareIO.h"

#ifdef USE_UVC_DEVICES
namespace r200
{
    class R200Camera : public rs::UVCCamera
    {
    public:
        R200Camera(uvc_context_t * ctx, uvc_device_t * device);

        float GetDepthScale() const override final { return 0.001f; } // convert mm to m
        void EnableStreamPreset(int streamIdentifier, int preset) override final;

        int GetStreamSubdeviceNumber(int stream) const override final;
        void RetrieveCalibration() override final;
        void SetStreamIntent(bool depth, bool color) override final;
    };
}
#endif

#endif
