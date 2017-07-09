// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once
#include <sensor_msgs/Image.h>
#include <realsense_msgs/frame_info.h>
#include "stream_data.h"
namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {

            struct image_info
            {
                std::string stream;
                uint32_t width;
                uint32_t height;
                rs::file_format::file_types::pixel_format format;
                uint32_t step;
                rs::file_format::file_types::nanoseconds capture_time;
                rs::file_format::file_types::seconds timestamp;
                rs::file_format::file_types::nanoseconds system_time;
                rs::file_format::file_types::timestamp_domain timestamp_domain;
                uint32_t device_id;
                uint32_t frame_number;
                std::shared_ptr <uint8_t> data;
                std::map <file_types::metadata_type, std::vector<uint8_t>> metadata;
            };


            class image : public sample
            {
            public:
                image(const image_info &info) :
                    m_info(info) {}


                file_types::sample_type get_type() const override
                {
                    return file_types::st_image;
                }

                status write_data(ros_writer& file) override
                {
                    sensor_msgs::Image image;
                    image.height = m_info.height;
                    image.width = m_info.width;
                    image.step = m_info.step;

                    if(conversions::convert(m_info.format, image.encoding) == false)
                    {
                        return status_param_unsupported;
                    }

                    image.is_bigendian = 0;

                    image.data.assign(m_info.data.get(), m_info.data.get() + (m_info.step * m_info.height));
                    image.header.seq = m_info.frame_number;
                    image.header.stamp = ros::Time(m_info.timestamp.count());

                    auto image_topic = get_topic(m_info.stream, m_info.device_id);

                    auto retval = file.write(image_topic, m_info.capture_time, image);
                    if(retval != status_no_error)
                    {
                        return retval;
                    }

                    realsense_msgs::frame_info msg;
                    msg.system_time = m_info.system_time.count();
                    msg.time_stamp_domain = m_info.timestamp_domain;

                    for(auto data : m_info.metadata)
                    {
                        realsense_msgs::metadata metadata;
                        metadata.type = data.first;
                        const uint8_t* raw_metadata = data.second.data();
                        metadata.data.assign(raw_metadata, raw_metadata + data.second.size());
                        msg.frame_metadata.push_back(metadata);
                    }

                    auto info_topic = get_info_topic(topic(image_topic).at(2), std::stoi(topic(image_topic).at(4)));
                    retval = file.write(info_topic, m_info.capture_time, msg);
                    if(retval != status_no_error)
                    {
                        return retval;
                    }
                    return status_no_error;
                }

                const image_info& get_info() const
                {
                    return m_info;
                }

                void set_info(const image_info& info)
                {
                    m_info = info;
                }

                static std::string get_topic(std::string stream, uint32_t device_id)
                {
                    return "/camera/" + stream + "/image_raw/" + std::to_string(device_id);
                }

                static std::string get_info_topic(std::string stream, uint32_t device_id)
                {
                    return "/camera/" + stream + "/rs_frame_info_ext/" + std::to_string(device_id);
                }
            private:
                image_info m_info;

            };
        }
    }
}
