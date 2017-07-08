// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "rs/storage/realsense_file/data_objects/time_sample.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {

            class time_sample_impl : public time_sample
            {
            private:
                time_sample_info m_info;
            public:
                time_sample_impl(const time_sample_info& info);

                virtual file_types::sample_type get_type() const override;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual time_sample_info get_info() const override;

                virtual void set_info(const time_sample_info& info) override;
            };
        }
    }
}



