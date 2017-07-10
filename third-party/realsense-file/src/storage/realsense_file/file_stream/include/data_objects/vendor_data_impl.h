// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once
#include "rs/storage/realsense_file/data_objects/vendor_data.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class vendor_data_impl : public vendor_data
            {
            private:
                vendor_info m_info;
            public:

                vendor_data_impl(const vendor_info& info);

                virtual ~vendor_data_impl();

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual vendor_info get_info() const override;

                virtual void set_info(const vendor_info& info) override;
            };
        }
    }
}


