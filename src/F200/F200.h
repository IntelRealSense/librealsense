#pragma once

#ifndef LIBREALSENSE_F200_CAMERA_H
#define LIBREALSENSE_F200_CAMERA_H

#include "../rs-internal.h"

namespace rsimpl { namespace f200
{
    class IVCAMHardwareIO;

    class F200Camera : public rs_camera
    {
        std::unique_ptr<IVCAMHardwareIO> hardware_io;

    public:
        
        F200Camera(uvc_context_t * ctx, uvc_device_t * device);
        ~F200Camera();

        CalibrationInfo RetrieveCalibration() override final;
        void SetStreamIntent() override final {}

        void EnableStreamPreset(rs_stream stream, rs_preset preset) override final
        {
            switch(stream)
            {
            case RS_STREAM_DEPTH: EnableStream(stream, 640, 480, RS_FORMAT_Z16, 60); break;
            case RS_STREAM_COLOR: EnableStream(stream, 640, 480, RS_FORMAT_RGB8, 60); break;
            default: throw std::runtime_error("unsupported stream");
            }
        }
    };
    
} } // namespace rsimpl::f200

#endif
