// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <media/ros/data_objects/stream_data.h>
#include <media/ros/data_objects/property.h>
#include <sensor_msgs/TimeReference.h>
namespace librealsense
{
    namespace file_format
    {
        namespace ros_data_objects
        {

            struct time_sample_info
            {
                file_types::nanoseconds capture_time;
                uint32_t device_id;
                std::string source;
                file_types::seconds timestamp;
                uint32_t frame_number;
            };

            class time_sample : public sample
            {
            public:
                time_sample(const time_sample_info &info): m_info(info) {}


                time_sample_info get_info() const
                {
                    return m_info;
                }

                void set_info(const time_sample_info &info)
                {
                    m_info = info;
                }

                file_types::sample_type get_type() const override
                {
                    return file_types::st_time;
                }

                void write_data(data_object_writer& file) override
                {
                    std::string topic = "/time/" + std::to_string(m_info.device_id);
                    sensor_msgs::TimeReference msg;
                    msg.header.seq = m_info.frame_number;
                    msg.header.stamp = ros::Time(m_info.timestamp.count());
                    msg.time_ref = ros::Time(m_info.timestamp.count());
                    msg.source = m_info.source;

                    file.write(topic, m_info.capture_time, msg);
                }

                static std::string get_topic(uint32_t device_id)
                {
                    return "/time/" + std::to_string(device_id);
                }

            private:
                time_sample_info m_info;
            };
        }
    }
}


