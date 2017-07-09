// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <ros/status.h>
#include <ros/ros_writer.h>

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class stream_data
            {
            public:
                virtual ~stream_data(){}
                virtual rs::file_format::status write_data(ros_writer& file) = 0;
            };

            struct stream_info : stream_data
            {
                virtual ~stream_info(){}
            };

            struct sample : stream_data
            {
                virtual ~sample(){}
                virtual file_types::sample_type get_type() const = 0;
            };
        }
    }
}
