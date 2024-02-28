// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "motion-dashboard.h"

using namespace rs2;
using namespace rsutils::string;

gyro_dashboard::gyro_dashboard( std::string name )
    : motion_dashboard( name )
{
}

void gyro_dashboard::process_frame( rs2::frame f )
{
    write_shared_data(
        [&]()
        {
            if( f && f.is< rs2::motion_frame >()
                && ( f.as< rs2::motion_frame >() ).get_profile().stream_type() == RS2_STREAM_GYRO )
            {
                double ts = glfwGetTime();
                auto it = frame_to_time.find( static_cast<float>( f.get_profile().unique_id() ) );

                if( ts - last_time > frame_rate && it != frame_to_time.end() )
                {
                    rs2::motion_frame frame = f.as< rs2::motion_frame >();

                    x_value = frame.get_motion_data().x;
                    y_value = frame.get_motion_data().y;
                    z_value = frame.get_motion_data().z;
                    n_value = std::sqrt( ( x_value * x_value ) + ( y_value * y_value ) + ( z_value * z_value ) );

                    if( x_history.size() > DEQUE_SIZE )
                        x_history.pop_front();
                    if( y_history.size() > DEQUE_SIZE )
                        y_history.pop_front();
                    if( z_history.size() > DEQUE_SIZE )
                        z_history.pop_front();
                    if( n_history.size() > DEQUE_SIZE )
                        n_history.pop_front();

                    x_history.push_back( x_value );
                    y_history.push_back( y_value );
                    z_history.push_back( z_value );
                    n_history.push_back( n_value );

                    last_time = ts;
                }

                frame_to_time[static_cast<float>( f.get_profile().unique_id() )] = ts;
            }
        } );
}