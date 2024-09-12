// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "depth-frame.h"
#include "sensor-interface.h"
#include "depth-sensor.h"
#include "extension.h"


namespace librealsense {


// Disparity frame provides an alternative representation of the depth data for stereo-based depth
// sensors In addition for the depth frame API it allows to query the stereoscopic baseline required
// to transform depth to disparity and vice versa
//
class disparity_frame : public depth_frame
{
public:
    disparity_frame()
        : depth_frame()
    {
    }

    // TODO Refactor to framemetadata
    float get_stereo_baseline( void ) const { return query_stereo_baseline( this->get_sensor() ); }

protected:
    static float query_stereo_baseline( const std::shared_ptr< sensor_interface > & sensor )
    {
        if( sensor != nullptr )
        {
            try
            {
                auto stereo_sensor = As< librealsense::depth_stereo_sensor >( sensor );
                if( stereo_sensor != nullptr )
                {
                    return stereo_sensor->get_stereo_baseline_mm();
                }
                else
                {
                    // For playback sensors
                    auto extendable = As< librealsense::extendable_interface >( sensor );
                    if( extendable
                        && extendable->extend_to( TypeToExtension< librealsense::depth_stereo_sensor >::value,
                                                  (void **)( &stereo_sensor ) ) )
                    {
                        return stereo_sensor->get_stereo_baseline_mm();
                    }
                }
            }
            catch( const std::exception & e )
            {
                LOG_ERROR( "Failed to query stereo baseline from sensor. " << e.what() );
            }
            catch( ... )
            {
                LOG_ERROR( "Failed to query stereo baseline from sensor" );
            }
        }
        else
        {
            LOG_WARNING( "sensor was nullptr" );
        }

        return 0;
    }
};

MAP_EXTENSION( RS2_EXTENSION_DISPARITY_FRAME, librealsense::disparity_frame );


}  // namespace librealsense
