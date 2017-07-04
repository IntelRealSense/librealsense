// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/msgs/rosgraph_msgs/Log.h"
#include "file_stream/include/topic.h"
#include "file_stream/include/data_objects/log_impl.h"
#include "file_stream/include/conversions.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;
using namespace rs::file_format::ros_data_objects;
using namespace rs::file_format::file_types;

std::string log::get_topic()
{
    return "/log";
}

std::shared_ptr<rs::file_format::ros_data_objects::log> rs::file_format::ros_data_objects::log::create(const log_info& info)
{
    return std::make_shared<log_impl>(info);
}

log_impl::log_impl(const log_info &info) : m_info(info) {}

file_types::sample_type log_impl::get_type() const { return file_types::st_log; }

status log_impl::write_data(std::shared_ptr<ros_writer> file)
{
    rosgraph_msgs::Log msg;
    msg.level = m_info.level;
    msg.msg = m_info.message;
    msg.file = m_info.file;
    msg.function = m_info.function;
    msg.line = m_info.line;

    auto file_instance = std::static_pointer_cast<ros_writer_impl>(file);
    return file_instance->write(get_topic(), m_info.capture_time, msg);
}

log_info log_impl::get_info() const
{
    return m_info;
}

void log_impl::set_info(const log_info &info)
{
    m_info = info;
}


