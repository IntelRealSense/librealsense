// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "include/data_objects/motion_sample.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class motion_sample_impl : public motion_sample
            {
            private:
                motion_info m_info;
            public:
                motion_sample_impl(const motion_info& info);

                virtual ~motion_sample_impl() = default;

                virtual file_types::sample_type get_type() const override;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual motion_info get_info() const override;

                virtual void set_info(const motion_info& info) override;

            };
        }
    }
}


