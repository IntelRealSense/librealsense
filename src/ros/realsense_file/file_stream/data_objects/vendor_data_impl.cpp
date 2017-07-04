// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/realsense_msgs/vendor_data.h"
#include "rosbag/view.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/ros_writer_impl.h"
#include "file_stream/include/data_objects/vendor_data_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string vendor_data::get_topic(const uint32_t& device_id)
{
    return "/info/" + std::to_string(device_id);
}

std::shared_ptr<vendor_data> vendor_data::create(const vendor_info& info)
{
    return std::make_shared<vendor_data_impl>(info);
}

vendor_data_impl::vendor_data_impl(const vendor_info &info): m_info(info){}

vendor_data_impl::~vendor_data_impl(){}

status vendor_data_impl::write_data(std::shared_ptr<ros_writer> file)
{
    realsense_msgs::vendor_data msg;
    msg.value = m_info.value;
    msg.name = m_info.name;
    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(m_info.device_id), file_types::nanoseconds::min(), msg);
}

vendor_info vendor_data_impl::get_info() const
{
    return m_info;
}

void vendor_data_impl::set_info(const vendor_info &info)
{
    m_info = info;
}

