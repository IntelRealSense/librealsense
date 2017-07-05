// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <memory>
#include <string>
#include "file_stream/include/ros_writer_impl.h"
//#include "rs_sdk_version.h"
using namespace rs::file_format;

ros_writer_impl::ros_writer_impl(std::string file_path)
{
    try
    {
        m_file = std::make_shared<rosbag::Bag>();
        m_file->open(file_path, rosbag::BagMode::Write);
        m_file->setCompression(rosbag::CompressionType::LZ4);
    } catch(rosbag::BagIOException&)
    {
        throw std::runtime_error("failed to open file");
    }
}

