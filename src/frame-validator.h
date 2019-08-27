// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "sensor.h"

namespace librealsense
{
    class frame_validator : public rs2_frame_callback
    {
    public:
        explicit frame_validator(std::shared_ptr<sensor_interface> sensor, frame_callback_ptr user_callback, stream_profiles current_requests);
        virtual ~frame_validator();

        void on_frame(rs2_frame * f) override;
        void release() override;
    private:
        bool propagate(frame_interface* frame);

        std::thread _reset_thread;
        frame_callback_ptr _user_callback;
        stream_profiles _current_requests;
        std::shared_ptr<sensor_interface> _sensor;
    };
}
