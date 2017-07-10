// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "stream_recorder.h"
#include "ros_writer.h"
#include <fstream>
#include <iostream>
#include <ros/data_objects/stream_data.h>

namespace rs
{
    namespace file_format
    {
        /**
        * @brief The stream_recorder provides an interface for recording realsense format files
        *
        */
        class stream_recorder
        {
        public:
            stream_recorder(const std::string& file_path);

            /**
            * @brief Writes a stream_data object to the file
            *
            * @param[in] data          an object implements the stream_data interface
            * @return status_no_error             Successful execution
            * @return status_param_unsupported    One of the stream data feilds is not supported
            */
            virtual void record(std::shared_ptr<ros_data_objects::stream_data> data);

//            std::string get_file_path();
        private:
            void write_file_version();
            ros_writer m_file;
        };
    }
}
