// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <std_msgs/Float64.h>
#include "stream_data.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {

            struct property_info
            {
                file_types::nanoseconds capture_time;
                uint32_t device_id;
                std::string key;
                double value;
            };

            class property : public sample
            {
            public:
                property(const property_info &info) : m_info(info) {}

                property_info get_info() const
                {
                    return m_info;
                }

                void set_info(const property_info &info)
                {
                    m_info = info;
                }


                file_types::sample_type get_type() const override
                {
                    return file_types::st_property;
                }

                status write_data(ros_writer& file) override
                {
                    std_msgs::Float64 msg;
                    msg.data = m_info.value;
                    return file.write(get_topic(m_info.key, m_info.device_id), m_info.capture_time, msg);
                }

                static std::string get_topic(const std::string& key, uint32_t device_id)
                {
                    return "/camera/property/" + key + "/" + std::to_string(device_id);
                }

            private:
                property_info m_info;
            };
        }
    }
}


