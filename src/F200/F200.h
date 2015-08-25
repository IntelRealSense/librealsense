#pragma once

#ifndef LIBREALSENSE_F200_CAMERA_H
#define LIBREALSENSE_F200_CAMERA_H

#include "../rs-internal.h"

namespace f200
{
    class IVCAMHardwareIO;

    class F200Camera : public rs_camera
    {
        std::unique_ptr<IVCAMHardwareIO> hardware_io;

    public:
        
        F200Camera(uvc_context_t * ctx, uvc_device_t * device);
        ~F200Camera();

        rs::CalibrationInfo RetrieveCalibration() override final;
        void SetStreamIntent() override final {}

        void EnableStreamPreset(int streamIdentifier, int preset) override final
        {
            switch(streamIdentifier)
            {
            case RS_DEPTH: EnableStream(RS_DEPTH, 640, 480, 60, RS_Z16); break;
            case RS_COLOR: EnableStream(RS_COLOR, 640, 480, 60, RS_RGB8); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }
    };
    
} // end namespace f200

#endif
