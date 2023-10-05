// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/h/rs_types.h>  // rs2_time_t

#include <chrono>


namespace librealsense {


class time_service
{
    time_service() = delete;  // not for instantiation

public:
    static rs2_time_t get_time()
    {
        return std::chrono::duration< double, std::milli >( std::chrono::system_clock::now().time_since_epoch() )
            .count();
    }
};


}  // namespace librealsense
