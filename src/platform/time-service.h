// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/h/rs_types.h>  // rs2_time_t

#include <chrono>


namespace librealsense {
namespace platform {


class time_service
{
public:
    virtual double get_time() const = 0;
    virtual ~time_service() = default;
};


class os_time_service : public time_service
{
public:
    rs2_time_t get_time() const override
    {
        return std::chrono::duration< double, std::milli >( std::chrono::system_clock::now().time_since_epoch() )
            .count();
    }
};


}  // namespace platform
}  // namespace librealsense
