// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/sensor_msgs/TimeReference.h"
#include "rosbag/view.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/data_objects/time_sample_impl.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string time_sample::get_topic(uint32_t device_id)
{
    return "/time/" + std::to_string(device_id);
}

std::shared_ptr<time_sample> time_sample::create(const time_sample_info& info)
{
    return std::make_shared<time_sample_impl>(info);
}

time_sample_impl::time_sample_impl(const time_sample_info &info): m_info(info) {}

file_types::sample_type time_sample_impl::get_type() const { return file_types::st_time; }

status time_sample_impl::write_data(std::shared_ptr<ros_writer> file)
{
    std::string topic = "/time/" + std::to_string(m_info.device_id);
    sensor_msgs::TimeReference msg;
    msg.header.seq = m_info.frame_number;
    msg.header.stamp = ros::Time(m_info.timestamp.count());
    msg.time_ref = ros::Time(m_info.timestamp.count());
    msg.source = m_info.source;

    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(topic, m_info.capture_time, msg);
}

time_sample_info time_sample_impl::get_info() const
{
    return m_info;
}

void time_sample_impl::set_info(const time_sample_info &info)
{
    m_info = info;
}

