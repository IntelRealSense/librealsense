// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "include/data_objects/motion_stream_info.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class motion_stream_info_impl : public motion_stream_info
            {
            private:
                motion_stream_data m_info;

                void assign(const motion_stream_data& in_info, motion_stream_data& out_info);

            public:
                motion_stream_info_impl(const motion_stream_data& info);

				virtual ~motion_stream_info_impl();

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual motion_stream_data get_info() const override;

                virtual void set_info(const motion_stream_data& info) override;

            };
        }
    }
}
