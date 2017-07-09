// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "stream_recorder.h"
#include "ros_writer.h"

#include "ros/file_format_version.h"
#include "ros_writer.h"
#include "std_msgs/UInt32.h"

void rs::file_format::stream_recorder::write_file_version()
{
    std_msgs::UInt32 msg;
    msg.data = get_file_version();

    if(m_file.write(get_file_version_topic(), file_types::nanoseconds::min(), msg) != status::status_no_error)
    {
        throw std::runtime_error("Failed to write file version");
    }
}

rs::file_format::stream_recorder::stream_recorder(const std::string &file_path) :
    m_file(file_path)
{
    write_file_version();
}

rs::file_format::status rs::file_format::stream_recorder::record(std::shared_ptr<rs::file_format::ros_data_objects::stream_data> data)
{
    return data->write_data(m_file);
}
