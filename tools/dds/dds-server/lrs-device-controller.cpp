// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "lrs-device-controller.h"
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <algorithm>
#include <iostream>

using namespace tools;

// This is a wrapper around a RealSense sensor, automatically opening and streaming it in the ctor
// and stopping/closing on destruction
class lrs_device_controller::lrs_sensor_streamer
{
public:
    lrs_sensor_streamer( rs2::sensor rs2_sensor,
                    rs2::stream_profile stream_profile,
                    frame_callback_type cb )
        : _rs2_sensor( rs2_sensor )
        , _stream_profile( stream_profile )
        , _frame_callback(std::move( cb ))
    {
        _rs2_sensor.open( _stream_profile );
        _rs2_sensor.start( [&]( rs2::frame f ) 
            {
                _frame_callback( f );
            } );
        std::cout << _stream_profile.stream_name() << " stream started"  << std::endl;
    }
    ~lrs_sensor_streamer()
    {
        _rs2_sensor.stop();
        _rs2_sensor.close();
        std::cout << _stream_profile.stream_name() << " stream stopped"  << std::endl;
    };

private:
    rs2::sensor _rs2_sensor;
    rs2::stream_profile _stream_profile;
    frame_callback_type _frame_callback;
};

lrs_device_controller::lrs_device_controller( rs2::device dev )
    : _rs_dev( dev )
{
    _device_sn = _rs_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    LOG_DEBUG( "LRS device manager for device: " << _device_sn << " created" );
}


lrs_device_controller::~lrs_device_controller()
{
    stop_all_streams();
    LOG_DEBUG( "LRS device manager for device: " << _device_sn << " deleted" );
}
void lrs_device_controller::start_stream( rs2::stream_profile sp, frame_callback_type cb )
{
    switch( sp.stream_type() )
    {
    case RS2_STREAM_COLOR: {
        auto cs = _rs_dev.first< rs2::color_sensor >();
        stream_to_rs2_sensor[RS2_STREAM_COLOR] = std::make_shared< lrs_sensor_streamer >( cs, sp, cb );
    }
    break;
    case RS2_STREAM_DEPTH: {
        auto ds = _rs_dev.first< rs2::depth_sensor >();
        stream_to_rs2_sensor[RS2_STREAM_DEPTH] = std::make_shared< lrs_sensor_streamer >( ds, sp, cb );
    }
    break;

    // TODO::: Add this streams
    case RS2_STREAM_INFRARED:
    case RS2_STREAM_FISHEYE:
    case RS2_STREAM_GYRO:
    case RS2_STREAM_ACCEL:
    case RS2_STREAM_GPIO:
    case RS2_STREAM_POSE:
    case RS2_STREAM_CONFIDENCE:
    default:
        throw std::runtime_error( "start_stream failed: unsupported stream: "
                                  + std::string( rs2_stream_to_string( sp.stream_type() ) ) );
    }
}

void lrs_device_controller::stop_stream( rs2_stream stream )
{
    auto sensor_to_erase_it = stream_to_rs2_sensor.find( stream );
    if( sensor_to_erase_it == stream_to_rs2_sensor.end() )
    {
        throw std::runtime_error( "Cannot stop stream:"
                                  + std::string( rs2_stream_to_string( stream ) ) + " as it is not streaming" );
    }

    stream_to_rs2_sensor.erase( sensor_to_erase_it );
}

void lrs_device_controller::stop_all_streams()
{
    stream_to_rs2_sensor.clear();
}
