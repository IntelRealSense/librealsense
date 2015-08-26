#pragma once

#ifndef LIBREALSENSE_R200_H
#define LIBREALSENSE_R200_H

#include "camera.h"

namespace rsimpl
{
    class r200_camera : public rs_camera
    {
    public:
        r200_camera(uvc_context_t * ctx, uvc_device_t * device);
        ~r200_camera();

        void enable_stream_preset(rs_stream stream, rs_preset preset) override final;
        calibration_info retrieve_calibration() override final;
        void set_stream_intent() override final;

        bool        set_disparity_shift(uvc_device_handle_t * device, uint32_t shift);
    };
}

#endif
