// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "frame.h"

namespace librealsense {
class composite_frame : public frame
{
public:
    composite_frame()
        : frame()
    {
    }

    frame_interface * get_frame( int i ) const
    {
        auto frames = get_frames();
        return frames[i];
    }

    frame_interface ** get_frames() const { return (frame_interface **)data.data(); }

    const frame_interface * first() const { return get_frame( 0 ); }
    frame_interface * first() { return get_frame( 0 ); }

    void keep() override
    {
        auto frames = get_frames();
        for( size_t i = 0; i < get_embedded_frames_count(); i++ )
            if( frames[i] )
                frames[i]->keep();
        frame::keep();
    }

    size_t get_embedded_frames_count() const { return data.size() / sizeof( rs2_frame * ); }

    // In the next section we make the composite frame "look and feel" like the first of its
    // children
    frame_header const & get_header() const override { return first()->get_header(); }
    rs2_metadata_type get_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const override
    {
        return first()->get_frame_metadata( frame_metadata );
    }
    bool supports_frame_metadata( const rs2_frame_metadata_value & frame_metadata ) const override
    {
        return first()->supports_frame_metadata( frame_metadata );
    }
    int get_frame_data_size() const override { return first()->get_frame_data_size(); }
    const byte * get_frame_data() const override { return first()->get_frame_data(); }
    rs2_time_t get_frame_timestamp() const override { return first()->get_frame_timestamp(); }
    rs2_timestamp_domain get_frame_timestamp_domain() const override
    {
        return first()->get_frame_timestamp_domain();
    }
    unsigned long long get_frame_number() const override
    {
        if( first() )
            return first()->get_frame_number();
        else
            return frame::get_frame_number();
    }
    rs2_time_t get_frame_system_time() const override { return first()->get_frame_system_time(); }
    std::shared_ptr< sensor_interface > get_sensor() const override
    {
        return first()->get_sensor();
    }
};

MAP_EXTENSION( RS2_EXTENSION_COMPOSITE_FRAME, librealsense::composite_frame );
}  // namespace librealsense
