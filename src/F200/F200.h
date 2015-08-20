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
        
        F200Camera(uvc_context_t * ctx, uvc_device_t * device);
        ~F200Camera();
        
        int GetStreamSubdeviceNumber(int stream) const override final;
        void RetrieveCalibration() override final;
        void SetStreamIntent(bool depth, bool color) override final {}

        void EnableStreamPreset(int streamIdentifier, int preset) override
        {
            switch(streamIdentifier)
            {
            case RS_DEPTH: EnableStream(RS_DEPTH, 640, 480, 60, RS_Z16); break;
            case RS_COLOR: EnableStream(RS_COLOR, 640, 480, 60, RS_RGB); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }
        
        rs_extrinsics GetStreamExtrinsics(int from, int to) override;
        void ComputeVertexImage() override;
    };
    
} // end namespace f200

#endif

#endif
