// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "sensor_msgs/Image.h"
#include "realsense_msgs/frame_info.h"
#include "realsense_msgs/compressed_frame_info.h"
#include "rosbag/view.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/data_objects/image_impl.h"
#include "file_stream/include/stream_recorder_impl.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string image::get_topic(std::string stream, uint32_t device_id)
{
    return "/camera/" + stream + "/image_raw/" + std::to_string(device_id);
}

std::string image::get_info_topic(std::string stream, uint32_t device_id)
{
    return "/camera/" + stream + "/rs_frame_info_ext/" + std::to_string(device_id);
}

std::shared_ptr<image> image::create(const image_info& info)
{
    return std::make_shared<image_impl>(info);
}

image_impl::image_impl(const image_info &info) :
    m_info(info) {}


image_impl::~image_impl()
{
}


file_types::sample_type image_impl::get_type() const { return file_types::st_image; }

status image_impl::write_data(std::shared_ptr<ros_writer> file)
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
    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    auto retval = file_instance->write(image_topic, m_info.capture_time, image);
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
    retval = file_instance->write(info_topic, m_info.capture_time, msg);
    if(retval != status_no_error)
    {
        return retval;
    }
    return status_no_error;
}

const image_info &image_impl::get_info() const
{
    return m_info;
}


void image_impl::set_info(const image_info &info)
{
    m_info = info;
}

