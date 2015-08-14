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
        R200Camera(uvc_context_t * ctx, uvc_device_t * device, int num);
        ~R200Camera();

        int GetDepthCameraNumber() const override final { return 1; }
        int GetColorCameraNumber() const override final { return 2; }
        void RetrieveCalibration() override final;

        void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
        void StartStreamPreset(int streamIdentifier, int preset) override
        {
            switch(streamIdentifier)
            {
            case RS_STREAM_DEPTH: StartStream(RS_STREAM_DEPTH, {628, 469, 0, rs::FrameFormat::Z16}); break;
            case RS_STREAM_RGB: StartStream(RS_STREAM_RGB, {640, 480, 59, rs::FrameFormat::YUYV}); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }
        void StopStream(int streamIdentifier) override;

        rs_intrinsics GetStreamIntrinsics(int stream) override;
        rs_extrinsics GetStreamExtrinsics(int from, int to) override;

        rs::StreamConfiguration GetZConfig() { return zConfig; }
        rs::StreamConfiguration GetRGBConfig() { return rgbConfig; }
        
        float GetDepthScale() override { return 0.001f; } // NOTE: Could change based on ZUnits
        const uint16_t * GetDepthImage() override;
        const uint8_t * GetColorImage() override;
    };
} // end namespace r200
#endif

#endif
