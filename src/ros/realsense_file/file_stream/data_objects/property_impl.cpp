// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "file_stream/include/data_objects/property_impl.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/ros_writer_impl.h"
#include "include/msgs/std_msgs/Float64.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string property::get_topic(const std::string& key, uint32_t device_id)
{
    return "/camera/property/" + key + "/" + std::to_string(device_id);
}

std::shared_ptr<property> property::create(const property_info& info)
{
    return std::make_shared<property_impl>(info);
}

property_impl::property_impl(const property_info &info) : m_info(info) {}

file_types::sample_type property_impl::get_type() const { return file_types::st_property; }

status property_impl::write_data(std::shared_ptr<ros_writer> file)
{
    std_msgs::Float64 msg;
    msg.data = m_info.value;
    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(m_info.key, m_info.device_id), m_info.capture_time, msg);
}

property_info property_impl::get_info() const
{
    return m_info;
}

void property_impl::set_info(const property_info &info)
{
    m_info = info;
}

