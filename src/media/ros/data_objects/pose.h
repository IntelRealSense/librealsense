// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <realsense_msgs/pose.h>
#include "stream_data.h"
namespace librealsense
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct pose_info
            {
				librealsense::float3 translation;
				librealsense::float4 rotation;
				librealsense::float3 velocity;
				librealsense::float3 angular_velocity;
				librealsense::float3 acceleration;
				librealsense::float3 angular_acceleration;
                file_types::nanoseconds capture_time;
                file_types::nanoseconds timestamp;
				file_types::nanoseconds system_timestamp;
				uint32_t device_id;

            };

            class pose : public sample
            {
            public:
                pose(const pose_info &info) : m_info(info)
                {
                }

                void write_data(data_object_writer& file) override
                {
                    realsense_msgs::pose msg;

                    msg.translation.x = m_info.translation.x;
                    msg.translation.y = m_info.translation.y;
                    msg.translation.z = m_info.translation.z;

                    msg.rotation.x = m_info.rotation.x;
                    msg.rotation.y = m_info.rotation.y;
                    msg.rotation.z = m_info.rotation.z;
                    msg.rotation.w = m_info.rotation.w;

                    msg.velocity.x = m_info.velocity.x;
                    msg.velocity.y = m_info.velocity.y;
                    msg.velocity.z = m_info.velocity.z;

                    msg.angular_velocity.x = m_info.angular_velocity.x;
                    msg.angular_velocity.y = m_info.angular_velocity.y;
                    msg.angular_velocity.z = m_info.angular_velocity.z;

                    msg.acceleration.x = m_info.acceleration.x;
                    msg.acceleration.y = m_info.acceleration.y;
                    msg.acceleration.z = m_info.acceleration.z;

                    msg.angular_acceleration.x = m_info.angular_acceleration.x;
                    msg.angular_acceleration.y = m_info.angular_acceleration.y;
                    msg.angular_acceleration.z = m_info.angular_acceleration.z;

                    msg.timestamp = m_info.timestamp.count();
                    msg.system_timestamp = m_info.system_timestamp.count();

                    file.write(get_topic(m_info.device_id), m_info.capture_time, msg);

                }

                file_types::sample_type get_type() const override
                {
                    return file_types::st_pose;
                }
                virtual pose_info get_info() const
                {
                    return m_info;
                }
                virtual void set_info(const pose_info& info)
                {
                    m_info = info;
                }
                static std::string get_topic(uint32_t device_id)
                {
                    return "/camera/rs_6DoF/" + std::to_string(device_id);
                }
            private:
                pose_info m_info;
            };
        }
    }
}

