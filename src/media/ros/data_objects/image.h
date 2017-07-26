// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once
#include "sensor_msgs/Image.h"
#include "realsense_msgs/frame_info.h"
#include "media/ros/topic.h"
#include "stream_data.h"
#include "core/serialization.h"
#include "archive.h"
#include "media/ros/file_types.h"
namespace librealsense
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class image : public sample
            {
            public:
                image(std::chrono::nanoseconds timestamp, uint32_t sensor_index, librealsense::frame_holder&& frame) :
                    m_timestamp(timestamp),
                    m_sensor_index(sensor_index),
                    m_frame(std::move(frame))
                {
                }

                file_types::sample_type get_type() const override
                {
                    return file_types::st_image;
                }

                void write_data(data_object_writer& file) override
                {
                    sensor_msgs::Image image;
                    auto vid_frame = dynamic_cast<librealsense::video_frame*>(m_frame.frame);
                    assert(vid_frame != nullptr);

                    std::string stream;
                   file_format::conversions::convert(vid_frame->get_stream_type(), stream);
                    
                    image.width = static_cast<uint32_t>(vid_frame->get_width());
                    image.height = static_cast<uint32_t>(vid_frame->get_height());
                    image.step = static_cast<uint32_t>(vid_frame->get_stride());
                    if(conversions::convert(vid_frame->get_format(), image.encoding) == false)
                    {
                        //return status_param_unsupported;
                        throw std::runtime_error("remove me, convert should throw");
                    }

                    image.is_bigendian = 0;
                    auto size = vid_frame->get_stride() * vid_frame->get_height();
                    auto p_data = vid_frame->get_frame_data();
                    image.data.assign(p_data, p_data + size);
                    image.header.seq = static_cast<uint32_t>(vid_frame->get_frame_number());

                    std::chrono::duration<double, std::milli> timestamp_ms(vid_frame->get_frame_timestamp());
                    auto timestamp =file_format::file_types::seconds(timestamp_ms);
                    image.header.stamp = ros::Time(timestamp.count());

                    auto image_topic = get_topic(stream, m_sensor_index);

                    file.write(image_topic, m_timestamp, image);


                    realsense_msgs::frame_info msg;
                    msg.system_time = 0;
                    msg.time_stamp_domain = vid_frame->get_frame_timestamp_domain();
                    std::map <rs2_frame_metadata, std::vector<uint8_t>> metadata;
                    copy_image_metadata(vid_frame, metadata);
                    for(auto data : metadata)
                    {
                        realsense_msgs::metadata metadata;
                        metadata.type = data.first;
                        const uint8_t* raw_metadata = data.second.data();
                        metadata.data.assign(raw_metadata, raw_metadata + data.second.size());
                        msg.frame_metadata.push_back(metadata);
                    }

                    auto info_topic = get_info_topic(topic(image_topic).at(2), std::stoi(topic(image_topic).at(4)));
                    file.write(info_topic, m_timestamp, msg);
                }

                void get_data(uint32_t& sensor_index, std::chrono::nanoseconds& timestamp, librealsense::frame_holder& frame)
                {
                    sensor_index = m_sensor_index;
                    timestamp = m_timestamp;
                    frame = std::move(m_frame);
                }

                static std::string get_topic(std::string stream, uint32_t device_id)
                {
                    return "/camera/" + stream + "/image_raw/" + std::to_string(device_id);
                }

                static std::string get_info_topic(std::string stream, uint32_t device_id)
                {
                    return "/camera/" + stream + "/rs_frame_info_ext/" + std::to_string(device_id);
                }

                static void copy_image_metadata(librealsense::video_frame* source,
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
                        memcpy(buffer.data(), &md, sizeof(md));
                        //TODO: Test above
                        buffer.swap(target[type]);
                    }
                    //TODO: Handle additional image metadata once available
                }
            private:
                std::chrono::nanoseconds m_timestamp;
                uint32_t m_sensor_index;
                librealsense::frame_holder m_frame;
            };
        }
    }
}
