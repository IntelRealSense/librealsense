#pragma once

#ifndef LIBREALSENSE_F200_CAMERA_H
#define LIBREALSENSE_F200_CAMERA_H

#include "../rs-internal.h"

#ifdef USE_UVC_DEVICES
#include "../UVCCamera.h"
#include "F200Types.h"
#include "HardwareIO.h"
#include "IVRectifier.h"

namespace f200
{
    
    class F200Camera : public rs::UVCCamera
    {
        
        std::unique_ptr<IVCAMHardwareIO> hardware_io;
        std::unique_ptr<IVRectifier> rectifier;
        
    public:
        
        F200Camera(uvc_context_t * ctx, uvc_device_t * device, int num);
        ~F200Camera();
        
        bool ConfigureStreams() override;
        void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
        void StartStreamPreset(int streamIdentifier, int preset) override
        {
            switch(streamIdentifier)
            {
            case RS_STREAM_DEPTH: StartStream(RS_STREAM_DEPTH, {640, 480, 60, rs::FrameFormat::INVZ}); break;
            case RS_STREAM_RGB: StartStream(RS_STREAM_RGB, {640, 480, 60, rs::FrameFormat::YUYV}); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }
        void StopStream(int streamIdentifier) override;
        
        rs_intrinsics GetStreamIntrinsics(int stream) override;
        rs_extrinsics GetStreamExtrinsics(int from, int to) override;
        
        const uint16_t * GetDepthImage() override;
        const uint8_t * GetColorImage() override;
        
    };
    
} // end namespace f200

#endif

#endif
