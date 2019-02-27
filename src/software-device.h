// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"
#include "device.h"
#include "context.h"

namespace librealsense
{
    class software_sensor;
    class software_device_info;

    class software_device : public device
    {
    public:
        software_device();

        software_sensor& add_software_sensor(const std::string& name);

        software_sensor& get_software_sensor(int index);

        void set_matcher_type(rs2_matchers matcher);
        
        std::shared_ptr<software_device_info> get_info();

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> markers;
            return markers;
        };
        void register_extrinsic(const stream_interface& stream, uint32_t groupd_index);

    private:
        std::vector<std::shared_ptr<software_sensor>> _software_sensors;
        rs2_matchers _matcher = RS2_MATCHER_DEFAULT;
        std::shared_ptr<software_device_info> _info;
    };
    
    class software_device_info : public device_info
    {
        std::weak_ptr<software_device> _dev;
    public:
        explicit software_device_info(std::shared_ptr<software_device> dev)
            : device_info(nullptr), _dev(dev)
        {
        }
        
        std::shared_ptr<device_interface> create_device(bool) const override
        {
            return _dev.lock();
        }
        platform::backend_device_group get_device_data() const override
        {
            std::stringstream address;
            address << "software-device://";
            if (auto dev = _dev.lock())
            {
                auto ptr = dev.get();
                address << (unsigned long long)ptr;
            }
            return platform::backend_device_group({ platform::playback_device_info{ address.str() } });
        }
        
        std::shared_ptr<device_interface> create(std::shared_ptr<context>, bool) const override
        {
            return _dev.lock();
        }
    };

    class software_sensor : public sensor_base
    {
    public:
        software_sensor(std::string name, software_device* owner);

        std::shared_ptr<stream_profile_interface> add_video_stream(rs2_video_stream video_stream);
        std::shared_ptr<stream_profile_interface> add_motion_stream(rs2_motion_stream motion_stream);
        std::shared_ptr<stream_profile_interface> add_pose_stream(rs2_pose_stream pose_stream);

        stream_profiles init_stream_profiles() override;

        void open(const stream_profiles& requests) override;
        void close() override;

        void start(frame_callback_ptr callback) override;
        void stop() override;

        void on_video_frame(rs2_software_video_frame frame);
        void on_motion_frame(rs2_software_motion_frame frame);
        void on_pose_frame(rs2_software_pose_frame frame);
        void add_read_only_option(rs2_option option, float val);
        void update_read_only_option(rs2_option option, float val);
        void set_metadata(rs2_frame_metadata_value key, rs2_metadata_type value);
    private:
        friend class software_device;
        stream_profiles _profiles;
        std::map<rs2_frame_metadata_value, rs2_metadata_type> _metadata_map;
        int _unique_id;
    };
    MAP_EXTENSION(RS2_EXTENSION_SOFTWARE_SENSOR, software_sensor);
    MAP_EXTENSION(RS2_EXTENSION_SOFTWARE_DEVICE, software_device);
}
