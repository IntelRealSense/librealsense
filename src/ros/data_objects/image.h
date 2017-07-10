// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once
#include "sensor_msgs/Image.h"
#include "realsense_msgs/frame_info.h"
#include "ros/conversions.h"
#include "ros/topic.h"
#include "stream_data.h"
#include "core/serialization.h"
#include "archive.h"
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
                rs2_format format;
                uint32_t step;
                rs::file_format::file_types::nanoseconds capture_time;
                rs::file_format::file_types::seconds timestamp;
                rs::file_format::file_types::nanoseconds system_time;
                rs2_timestamp_domain timestamp_domain;
                uint32_t device_id;
                uint32_t frame_number;
                std::shared_ptr <uint8_t> data;
                std::map <rs2_frame_metadata, std::vector<uint8_t>> metadata;
            };


            class image : public sample
            {
            public:
                image(const image_info &info) :
                    m_info(info) {

                }
                image(const librealsense::device_serializer::frame_box& data)
                {
                    auto vid_frame = std::dynamic_pointer_cast<librealsense::video_frame>(data.frame);
                    assert(vid_frame != nullptr);
                    auto buffer_size = vid_frame->data.size();
                    std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>(new uint8_t[buffer_size],
                                                                               [](uint8_t* ptr){delete[] ptr;});
                    memcpy(buffer.get(), vid_frame->data.data(), vid_frame->data.size());
                    rs::file_format::ros_data_objects::image_info info;
                    info.device_id = data.sensor_index;
                    rs::file_format::conversions::convert(vid_frame->get_stream_type(), info.stream);
                    info.width = static_cast<uint32_t>(vid_frame->get_width());
                    info.height = static_cast<uint32_t>(vid_frame->get_height());
                    info.format = vid_frame->get_format();
                    info.step = static_cast<uint32_t>(vid_frame->get_stride()); //TODO: Ziv, assert stride == pitch
                    info.capture_time = data.timestamp;
                    info.timestamp_domain = vid_frame->get_frame_timestamp_domain();
                    std::chrono::duration<double, std::milli> timestamp_ms(vid_frame->get_frame_timestamp());
                    info.timestamp = rs::file_format::file_types::seconds(timestamp_ms);
                    info.frame_number = static_cast<uint32_t>(vid_frame->get_frame_number());
                    info.data = buffer;
                    copy_image_metadata(vid_frame, info.metadata);
                }

                file_types::sample_type get_type() const override
                {
                    return file_types::st_image;
                }

                void write_data(ros_writer& file) override
                {
                    sensor_msgs::Image image;
                    image.height = m_info.height;
                    image.width = m_info.width;
                    image.step = m_info.step;

                    if(conversions::convert(m_info.format, image.encoding) == false)
                    {
                        //return status_param_unsupported;
                    }

                    image.is_bigendian = 0;

                    image.data.assign(m_info.data.get(), m_info.data.get() + (m_info.step * m_info.height));
                    image.header.seq = m_info.frame_number;
                    image.header.stamp = ros::Time(m_info.timestamp.count());

                    auto image_topic = get_topic(m_info.stream, m_info.device_id);

                    file.write(image_topic, m_info.capture_time, image);


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
                    file.write(info_topic, m_info.capture_time, msg);
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

                static bool copy_image_metadata(std::shared_ptr<librealsense::video_frame> source,
                                                std::map<rs2_frame_metadata, std::vector<uint8_t>>& target)
                {
                    for (int i = 0; i < static_cast<rs2_frame_metadata>(rs2_frame_metadata::RS2_FRAME_METADATA_COUNT); i++)
                    {
                        rs2_frame_metadata type = static_cast<rs2_frame_metadata>(i);
                        if(!source->supports_frame_metadata(type))
                        {
                            continue;
                        }
                        auto md = source->get_frame_metadata(type);
                        std::vector<uint8_t> buffer(sizeof(md));
                        mempcpy(buffer.data(), &md, sizeof(md));
                        //TODO: Test above
                        buffer.swap(target[type]);
                    }
                    //TODO: Handle additional image metadata once available
                }
            private:
                image_info m_info;

            };
        }
    }
}
