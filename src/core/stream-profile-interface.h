// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "stream-interface.h"
#include "extension.h"

#include <librealsense2/h/rs_sensor.h>
#include <functional>
#include <memory>
#include <vector>
#include <ostream>


namespace librealsense {


class stream_profile_interface
    : public stream_interface
    , public recordable< stream_profile_interface >
{
public:
    virtual rs2_format get_format() const = 0;
    virtual void set_format( rs2_format format ) = 0;

    virtual uint32_t get_framerate() const = 0;
    virtual void set_framerate( uint32_t val ) = 0;

    virtual int get_tag() const = 0;
    virtual void tag_profile( int tag ) = 0;

    virtual std::shared_ptr< stream_profile_interface > clone() const = 0;
    virtual rs2_stream_profile * get_c_wrapper() const = 0;
    virtual void set_c_wrapper( rs2_stream_profile * wrapper ) = 0;
};


using stream_profiles = std::vector< std::shared_ptr< stream_profile_interface > >;


inline std::ostream & operator<<( std::ostream & os, const stream_profiles & profiles )
{
    for( auto & p : profiles )
    {
        os << rs2_format_to_string( p->get_format() ) << " " << rs2_stream_to_string( p->get_stream_type() ) << ", ";
    }
    return os;
}


}  // namespace librealsense
