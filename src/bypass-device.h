// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"
#include "core/video.h"
#include "device.h"

#include <string>

namespace librealsense
{
    class bypass_sensor;

    class bypass_device : public device
    {
    public:
        bypass_device();

        void add_bypass_sensor(const std::string& name);

        bypass_sensor& get_bypass_sensor(int index);

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    private:
        std::vector<std::shared_ptr<bypass_sensor>> _bypass_sensors;
    };

    class bypass_sensor : public sensor_base
    {
    public:
        bypass_sensor(std::string name, bypass_device* owner);

        void add_video_stream(rs2_stream type, int index, int uid, int width, int height, int fps, rs2_format fmt);

        stream_profiles init_stream_profiles() override;

        void open(const stream_profiles& requests) override;
        void close() override;

        void start(frame_callback_ptr callback) override;
        void stop() override;

        void on_video_frame(void* pixels, 
            void(*deleter)(void*),
            rs2_time_t ts, rs2_timestamp_domain domain,
            int frame_number,
            stream_profile_interface* profile);

    private:
        friend class bypass_device;
        stream_profiles _profiles;
    };

    MAP_EXTENSION(RS2_EXTENSION_BYPASS_DEVICE, bypass_device);
}
