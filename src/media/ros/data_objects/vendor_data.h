// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <media/ros/data_objects/stream_data.h>
#include "realsense_msgs/vendor_data.h"
namespace librealsense
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct vendor_info
            {
                std::string name;
                std::string value;
                uint32_t device_id;
            };


            class vendor_data : public stream_data
            {
            public:
                vendor_data(const vendor_info &info): m_info(info){}

                vendor_info get_info() const
                {
                    return m_info;
                }

                void set_info(const vendor_info &info)
                {
                    m_info = info;
                }

                void write_data(data_object_writer& file) override
                {
                    realsense_msgs::vendor_data msg;
                    msg.value = m_info.value;
                    msg.name = m_info.name;
                    file.write(get_topic(m_info.device_id), file_types::nanoseconds::min(), msg);
                }

                static std::string get_topic(const uint32_t& device_id = -1)
                {
                    return "/info/" + std::to_string(device_id);
                }
            private:
                vendor_info m_info;
            };
        }
    }
}


