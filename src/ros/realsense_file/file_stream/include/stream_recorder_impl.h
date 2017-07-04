// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <fstream>
#include <iostream>
#include <include/status.h>
#include "include/stream_recorder.h"
#include "include/ros_writer.h"
namespace rs
{
    namespace file_format
    {
        class stream_recorder_impl : public rs::file_format::stream_recorder
        {
        public:
            stream_recorder_impl(const std::string& file_path);

            virtual ~stream_recorder_impl();

            virtual rs::file_format::status record(std::shared_ptr<ros_data_objects::stream_data> data);

        private:
            bool write_file_version();
            std::shared_ptr<ros_writer>    m_file;
        };
    }
}
