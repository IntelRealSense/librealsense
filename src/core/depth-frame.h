// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "video-frame.h"
#include "extension.h"


namespace librealsense {


class depth_frame : public video_frame
{
public:
    depth_frame()
        : video_frame()
    {
    }

    frame_interface * publish( std::shared_ptr< archive_interface > new_owner ) override
    {
        return video_frame::publish( new_owner );
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
    static float query_units( const std::shared_ptr< sensor_interface > & sensor )
    {
        if( sensor != nullptr )
        {
            try
            {
                auto depth_sensor = As< librealsense::depth_sensor >( sensor );
                if( depth_sensor != nullptr )
                {
                    return depth_sensor->get_depth_scale();
                }
                else
                {
                    // For playback sensors
                    auto extendable = As< librealsense::extendable_interface >( sensor );
                    if( extendable
                        && extendable->extend_to( TypeToExtension< librealsense::depth_sensor >::value,
                                                  (void **)( &depth_sensor ) ) )
                    {
                        return depth_sensor->get_depth_scale();
                    }
                }
            }
            catch( const std::exception & e )
            {
                LOG_ERROR( "Failed to query depth units from sensor. " << e.what() );
            }
            catch( ... )
            {
                LOG_ERROR( "Failed to query depth units from sensor" );
            }
        }
        else
        {
            LOG_WARNING( "sensor was nullptr" );
        }

        return 0;
    }

    frame_holder _original;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_FRAME, librealsense::depth_frame );


}  // namespace librealsense
