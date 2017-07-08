// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "rs/storage/realsense_file/data_objects/image_stream_info.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class image_stream_info_impl : public image_stream_info
            {
            private:
                image_stream_data m_info;
                void assign(const image_stream_data& in_info, image_stream_data& out_info);

            public:
                image_stream_info_impl(const image_stream_data& info);

                virtual ~image_stream_info_impl() = default;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual const image_stream_data& get_info() const override;

                virtual void set_info(const image_stream_data& info) override;
            };
        }
    }
}
