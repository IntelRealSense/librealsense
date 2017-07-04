// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "include/data_objects/pose.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class pose_impl : public pose
            {
            private:
                pose_info m_info;
            public:
                pose_impl(const pose_info& info);

                virtual ~pose_impl() = default;

                virtual file_types::sample_type get_type() const override;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual pose_info get_info() const override;

                virtual void set_info(const pose_info& info) override;

            };
        }
    }
}


