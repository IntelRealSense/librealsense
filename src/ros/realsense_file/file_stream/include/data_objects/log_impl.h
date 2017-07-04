// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "include/data_objects/log.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class log_impl : public log
            {
            private:
                log_info m_info;
            public:
                log_impl(const log_info& info);

                virtual ~log_impl() = default;

                virtual file_types::sample_type get_type() const override;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual log_info get_info() const override;

                virtual void set_info(const log_info& info) override;

            };
        }
    }
}


