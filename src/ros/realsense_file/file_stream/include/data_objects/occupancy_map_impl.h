// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "include/data_objects/occupancy_map.h"

namespace rs
{
    namespace file_format
    {
        namespace ros_data_objects
        {
            class occupancy_map_impl : public occupancy_map
            {
            private:
                occupancy_map_info m_info;
            public:
                occupancy_map_impl(const occupancy_map_info& info);

                virtual ~occupancy_map_impl() = default;

                virtual file_types::sample_type get_type() const override;

                virtual status write_data(std::shared_ptr<ros_writer> file) override;

                virtual occupancy_map_info get_info() const override;

                virtual void set_info(const occupancy_map_info& info) override;

            };
        }
    }
}


