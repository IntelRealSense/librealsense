// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"
#include "core/video.h"
#include "device.h"
#include "device.h"
#include "context.h"

namespace librealsense
{
    class bypass_sensor;

    class bypass_device : public device
    {
    public:
        bypass_device();

        bypass_sensor& add_bypass_sensor(const std::string& name);

        bypass_sensor& get_bypass_sensor(int index);

        void set_matcher_type(rs2_matchers matcher);

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;
    private:
        std::vector<std::shared_ptr<bypass_sensor>> _bypass_sensors;
        rs2_matchers _matcher = DEFAULT;
    };

    class bypass_sensor : public sensor_base
    {
    public:
        bypass_sensor(std::string name, bypass_device* owner);

        void add_video_stream(rs2_video_stream video_stream);

        stream_profiles init_stream_profiles() override;

        void open(const stream_profiles& requests) override;
        void close() override;

        void start(frame_callback_ptr callback) override;
        void stop() override;

        void on_video_frame(rs2_bypass_video_frame frame);
        void add_read_only_option(rs2_option option, float val);
        void update_read_only_option(rs2_option option, float val);

    private:
        friend class bypass_device;
        stream_profiles _profiles;
    };
    MAP_EXTENSION(RS2_EXTENSION_BYPASS_SENSOR, bypass_sensor);
    MAP_EXTENSION(RS2_EXTENSION_BYPASS_DEVICE, bypass_device);
}
