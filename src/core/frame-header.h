// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/h/rs_frame.h>
#include <iosfwd>


namespace librealsense {


/*
    Each frame is attached with a static header
    This is a quick and dirty way to manage things like timestamp,
    frame-number, metadata, etc... Things shared between all frame extensions
    The point of this class is to be **fixed-sized**, avoiding per frame allocations
*/
struct frame_header
{
    unsigned long long frame_number = 0;
    rs2_time_t timestamp = 0;
    rs2_timestamp_domain timestamp_domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
    rs2_time_t system_time = 0;        // sys-clock at the time the frame was received from the backend
    rs2_time_t backend_timestamp = 0;  // time when the frame arrived to the backend (OS dependent)

    frame_header() = default;
    frame_header( frame_header const & ) = default;
    frame_header( rs2_time_t in_timestamp,
                  unsigned long long in_frame_number,
                  rs2_time_t in_system_time,
                  rs2_time_t backend_time )
        : timestamp( in_timestamp )
        , frame_number( in_frame_number )
        , system_time( in_system_time )
        , backend_timestamp( backend_time )
    {
    }
};

std::ostream & operator<<( std::ostream & os, frame_header const & header );


}  // namespace librealsense
