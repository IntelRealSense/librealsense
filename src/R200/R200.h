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
    public:
        R200Camera(uvc_context_t * ctx, uvc_device_t * device);
        ~R200Camera();

        int GetStreamSubdeviceNumber(int stream) const override final;
        void RetrieveCalibration() override final;
        void SetStreamIntent(bool depth, bool color) override final;

        void EnableStreamPreset(int streamIdentifier, int preset) override final
        {
            switch(streamIdentifier)
            {
            case RS_DEPTH: EnableStream(RS_DEPTH, 480, 360, 0, RS_Z16); break;
            case RS_COLOR: EnableStream(RS_COLOR, 640, 480, 60, RS_RGB); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }

        rs_extrinsics GetStreamExtrinsics(int from, int to) override final;
        float GetDepthScale() const override final { return 0.001f; } // convert mm to m
    };
} // end namespace r200
#endif

#endif
