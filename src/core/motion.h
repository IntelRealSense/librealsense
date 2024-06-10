// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "stream-profile-interface.h"
#include <src/float3.h>


namespace librealsense
{
    class motion_stream_profile_interface : public virtual stream_profile_interface
    {
    public:
        virtual rs2_motion_device_intrinsic get_intrinsics() const = 0;
        virtual void set_intrinsics(std::function<rs2_motion_device_intrinsic()> calc) = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_MOTION_PROFILE, librealsense::motion_stream_profile_interface);

    class pose_stream_profile_interface : public virtual stream_profile_interface
    {
        //Empty for now
    };

    MAP_EXTENSION(RS2_EXTENSION_POSE_PROFILE, librealsense::pose_stream_profile_interface);

    class pose_sensor_interface
    {
    public:
        virtual bool export_relocalization_map(std::vector<uint8_t>& lmap_buf) const  = 0;
        virtual bool import_relocalization_map(const std::vector<uint8_t>& lmap_buf) const = 0;
        virtual bool set_static_node(const std::string& guid, const float3& pos, const float4& orient_quat) const = 0;
        virtual bool get_static_node(const std::string& guid, float3& pos, float4& orient_quat) const = 0;
        virtual bool remove_static_node(const std::string& guid) const = 0;
        virtual ~pose_sensor_interface() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_POSE_SENSOR, librealsense::pose_sensor_interface);

    class wheel_odometry_interface
    {
    public:
        virtual bool load_wheel_odometery_config(const std::vector<uint8_t>& odometry_config_buf) const = 0;
        virtual bool send_wheel_odometry(uint8_t wo_sensor_id, uint32_t frame_num, const float3& translational_velocity) const = 0;
        virtual ~wheel_odometry_interface() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_WHEEL_ODOMETER, librealsense::wheel_odometry_interface);
}
