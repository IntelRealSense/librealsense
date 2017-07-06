// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "include/stream_recorder_impl.h"
#include "file_stream/include/ros_writer_impl.h"
#include "std_msgs/UInt32.h"
#include "file_stream/include/file_format_version.h"
#include "file_stream/include/ros_writer_impl.h"

using namespace rs::file_format;

bool stream_recorder::create(const std::string& file_path, std::unique_ptr<stream_recorder>& recorder)
{
    try
    {
        recorder = std::unique_ptr<stream_recorder_impl>(new stream_recorder_impl(file_path));
    }
    catch (std::exception&)
    {
        return false;
    }
	return true;
}

bool stream_recorder_impl::write_file_version()
{
    std_msgs::UInt32 msg;
    msg.data = get_file_version();

    auto file_instance = std::static_pointer_cast<ros_writer_impl>(m_file);

    if(file_instance->write(get_file_version_topic(), file_types::nanoseconds::min(), msg) != status::status_no_error)
    {
        return false;
    }
    return true;
}

stream_recorder_impl::stream_recorder_impl(const std::string &file_path)
{
    if (file_path.empty())
    {
        throw std::invalid_argument("file_path");
    }
    m_file = std::make_shared<ros_writer_impl>(file_path);

    if(write_file_version() == false)
    {
        throw std::runtime_error("failed to write file version");
    }

}

stream_recorder_impl::~stream_recorder_impl()
{
	m_file.reset();
}

status stream_recorder_impl::record(std::shared_ptr<ros_data_objects::stream_data> data)
{
    return data->write_data(m_file);
}
