// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <librealsense2/h/rs_sensor.h>
#include <memory>
#include <vector>


namespace librealsense {


class stream_interface : public std::enable_shared_from_this< stream_interface >
{
public:
    virtual ~stream_interface() = default;

    virtual int get_stream_index() const = 0;
    virtual void set_stream_index( int index ) = 0;

    virtual int get_unique_id() const = 0;
    virtual void set_unique_id( int uid ) = 0;

    virtual rs2_stream get_stream_type() const = 0;
    virtual void set_stream_type( rs2_stream stream ) = 0;
};


// TODO this should be find_stream
stream_interface * find_profile( rs2_stream stream, int index, std::vector< stream_interface * > const & profiles );


}  // namespace librealsense
