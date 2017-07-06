// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "rs/storage/realsense_file/stream_recorder.h"
#include "rs/storage/realsense_file/ros_writer.h"
#include <fstream>
#include <iostream>

namespace rs
{
    namespace file_format
    {
        class stream_recorder_impl : public stream_recorder
        {
        public:
            stream_recorder_impl(const std::string& file_path);

            virtual ~stream_recorder_impl();

            virtual status record(std::shared_ptr<ros_data_objects::stream_data> data);

        private:
            bool write_file_version();
            std::shared_ptr<ros_writer>    m_file;
        };
    }
}
