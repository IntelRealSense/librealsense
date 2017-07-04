// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/realsense_msgs/compressed_frame_info.h"
#include "include/msgs/sensor_msgs/CompressedImage.h"
#include "rosbag/view.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/data_objects/compressed_image_impl.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;

std::string compressed_image::get_topic(std::string stream, uint32_t device_id)
{
    return "/camera/" + stream + "/compressed_image/" + std::to_string(device_id);
}

std::string compressed_image::get_info_topic(std::string stream, uint32_t device_id)
{
    return "/camera/" + stream + "/rs_compressed_frame_info_ext/" + std::to_string(device_id);
}

std::shared_ptr<compressed_image> compressed_image::create(const compressed_image_info& info)
{
    return std::make_shared<compressed_image_impl>(info);
}

compressed_image_impl::compressed_image_impl(const compressed_image_info &info) : m_info(info){}

compressed_image_impl::~compressed_image_impl()
{
}

file_types::sample_type compressed_image_impl::get_type() const { return file_types::st_compressed_image; }

status compressed_image_impl::write_data(std::shared_ptr<ros_writer> file)
{

    sensor_msgs::CompressedImage image;

    if(conversions::convert(m_info.compression_type, image.format) == false)
    {
        return status_param_unsupported;
    }
    image.data.assign(m_info.data.get(), m_info.data.get() + (m_info.compression_size));
    image.header.seq = m_info.frame_number;
    image.header.stamp = ros::Time(m_info.timestamp.count());

    auto image_topic = get_topic(m_info.stream, m_info.device_id);
    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    auto retval = file_instance->write(image_topic, m_info.capture_time, image);
    if(retval != status_no_error)
    {
        return retval;
    }
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
        return status_item_unavailable;
    }
    auto info_topic = get_info_topic(topic(image_topic).at(2), std::stoi(topic(image_topic).at(4)));
    retval = file_instance->write(info_topic, m_info.capture_time, msg);
    if(retval != status_no_error)
    {
        return retval;
    }
    return status_no_error;
}

compressed_image_info compressed_image_impl::get_info() const
{
    return m_info;
}

void compressed_image_impl::set_info(const compressed_image_info &info)
{
    m_info = info;
}
