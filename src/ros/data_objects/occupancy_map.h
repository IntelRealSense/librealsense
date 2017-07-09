// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <realsense_msgs/occupancy_map.h>
#include "stream_data.h"
namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct occupancy_map_info
            {
                uint8_t accuracy;
                uint8_t reserved;
                uint16_t tile_count;
                std::shared_ptr<float> tiles;
                file_types::nanoseconds capture_time;
                uint32_t device_id;

            };

            class occupancy_map : public sample
            {
            public:
                occupancy_map(const occupancy_map_info &info) : m_info(info) {}

                virtual occupancy_map_info get_info() const
                {
                    return m_info;
                }
                virtual void set_info(const occupancy_map_info& info)
                {
                    m_info = info;
                }

                file_types::sample_type get_type() const override
                {
                    return file_types::st_occupancy_map;
                }
                status write_data(ros_writer& file) override
                {
                    realsense_msgs::occupancy_map msg;
                    msg.accuracy = m_info.accuracy;
                    msg.reserved = m_info.reserved;
                    msg.tile_count = m_info.tile_count;
                    msg.tiles.assign(m_info.tiles.get(), m_info.tiles.get() + (m_info.tile_count * 3));
                    return file.write(get_topic(m_info.device_id), m_info.capture_time, msg);
                }
                static std::string get_topic(uint32_t device_id)
                {
                    return "/camera/rs_occupancy_map/" + std::to_string(device_id);
                }
            private:
                occupancy_map_info m_info;
            };
        }
    }
}


