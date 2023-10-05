// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "video-frame.h"
#include "frame-holder.h"
#include "extension.h"


namespace librealsense {


class sensor_interface;
class archive_interface;


class depth_frame : public video_frame
{
public:
    depth_frame()
        : video_frame()
    {
    }

    void keep() override
    {
        if( _original )
            _original->keep();
        video_frame::keep();
    }

    float get_distance( int x, int y ) const;

    float get_units() const { return additional_data.depth_units; }

    void set_original( frame_holder h )
    {
        _original = std::move( h );
        attach_continuation( frame_continuation(
            [this]()
            {
                if( _original )
                {
                    _original = {};
                }
            },
            nullptr ) );
    }

protected:
    frame_holder _original;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_FRAME, librealsense::depth_frame );


}  // namespace librealsense
