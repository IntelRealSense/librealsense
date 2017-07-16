// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <map>
#include <ros/file_types.h>
#include <memory>
#include <vector>
#include "ros/status.h"
#include "sensor_msgs/CompressedImage.h"
#include "realsense_msgs/compressed_frame_info.h"
#include "ros/topic.h"
#include "stream_data.h"


namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            struct compressed_image_info
            {
                std::string stream;
                uint32_t width;
                uint32_t height;
                rs2_format format;
                uint32_t step;
                file_types::nanoseconds capture_time;
                file_types::seconds timestamp;
                file_types::nanoseconds system_time;
                rs2_timestamp_domain timestamp_domain;
                uint32_t device_id;
                uint32_t frame_number;
                std::shared_ptr<uint8_t> data;
                file_types::compression_type compression_type;
                uint32_t compression_size;
                std::map<rs2_frame_metadata, std::vector<uint8_t>> metadata;
            };

            class compressed_image : public rs::file_format::ros_data_objects::sample
            {
            public:
                compressed_image(const compressed_image_info &info) :
                    m_info(info)
                {

                }


                rs::file_format::file_types::sample_type get_type() const override
                {
                    return file_types::st_compressed_image;
                }


                void write_data(ros_writer& file) override
                {

                    sensor_msgs::CompressedImage image;

                    if(rs::file_format::conversions::convert(m_info.compression_type, image.format) == false)
                    {
                        //return status_param_unsupported;
                    }
                    image.data.assign(m_info.data.get(), m_info.data.get() + (m_info.compression_size));
                    image.header.seq = m_info.frame_number;
                    image.header.stamp = ros::Time(m_info.timestamp.count());

                    auto image_topic = get_topic(m_info.stream, m_info.device_id);

                    file.write(image_topic, m_info.capture_time, image);

                    realsense_msgs::compressed_frame_info msg;
                    msg.system_time = m_info.system_time.count();
                    msg.time_stamp_domain = m_info.timestamp_domain;
                    msg.height = m_info.height;
                    msg.width = m_info.width;
                    msg.step = m_info.step;

                    for(auto data : m_info.metadata)
                    {
                        realsense_msgs::metadata metadata;
                        metadata.type = data.first;
                        const uint8_t* raw_metadata = data.second.data();
                        metadata.data.assign(raw_metadata, raw_metadata + data.second.size());
                        msg.frame_metadata.push_back(metadata);
                    }

                    if(conversions::convert(m_info.format, msg.encoding) == false)
                    {
                        //return status_item_unavailable;
                    }
                    auto info_topic = get_info_topic(topic(image_topic).at(2), std::stoi(topic(image_topic).at(4)));
                    file.write(info_topic, m_info.capture_time, msg);

                    //return status_no_error;
                }

                compressed_image_info get_info() const
                {
                    return m_info;
                }

                void set_info(const compressed_image_info &info)
                {
                    m_info = info;
                }

                static std::string get_topic(std::string stream, uint32_t device_id)
                {
                    return "/camera/" + stream + "/compressed_image/" + std::to_string(device_id);
                }

                static std::string get_info_topic(std::string stream, uint32_t device_id)
                {
                    return "/camera/" + stream + "/rs_compressed_frame_info_ext/" + std::to_string(device_id);
                }


            private:
                compressed_image_info m_info;

            };
        }
    }
}
