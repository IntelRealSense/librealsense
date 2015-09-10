#pragma once
#ifndef LIBREALSENSE_R200_H
#define LIBREALSENSE_R200_H

#include "device.h"

namespace rsimpl
{
    class r200_camera : public rs_device
    {
    public:
        r200_camera(uvc::device device);
        ~r200_camera();

        void set_stream_intent(const bool (& stream_enabled)[RS_STREAM_COUNT]) override final;
        void set_option(rs_option option, int value) override final;
        int get_option(rs_option option) const override final;
        int convert_timestamp(int64_t timestamp) const override final;
    };
}

#endif
