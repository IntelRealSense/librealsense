#pragma once
#ifndef LIBREALSENSE_R200_H
#define LIBREALSENSE_R200_H

#include "camera.h"

namespace rsimpl
{
    class r200_camera : public rs_camera
    {
    public:
        r200_camera(uvc_context_t * ctx, uvc::device device);
        ~r200_camera();

        calibration_info retrieve_calibration() override final;
        void set_stream_intent() override final;
        void set_option(rs_option option, int value) override final;
        int get_option(rs_option option) override final;
    };
}

#endif
