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
        R200Camera(uvc_context_t * ctx, uvc_device_t * device, int num);
        ~R200Camera();

        int GetDepthCameraNumber() const override final { return 1; }
        int GetColorCameraNumber() const override final { return 2; }
        void RetrieveCalibration() override final;
        void SetStreamIntent(bool depth, bool color) override final;

        void EnableStreamPreset(int streamIdentifier, int preset) override
        {
            switch(streamIdentifier)
            {
            case RS_DEPTH: EnableStream(RS_DEPTH, 480, 360, 0, RS_Z16); break;
            case RS_COLOR: EnableStream(RS_COLOR, 640, 480, 60, RS_RGB); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }

        rs_extrinsics GetStreamExtrinsics(int from, int to) override;
        void ComputeVertexImage() override;
    };
} // end namespace r200
#endif

#endif
