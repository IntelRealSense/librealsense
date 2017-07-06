// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "rs/storage/realsense_file/data_objects/property.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class property_impl : public property
            {
            private:
                property_info m_info;
            public:
                property_impl(const property_info& info);

                virtual ~property_impl() = default;

                virtual file_types::sample_type get_type() const override;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual property_info get_info() const override;

                virtual void set_info(const property_info& info) override;
            };
        }
    }
}


