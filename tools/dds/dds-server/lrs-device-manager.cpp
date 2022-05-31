// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include <iostream>
#include "lrs-device-manager.h"

using namespace tools;

class lrs_device_manager::lrs_sensor_streamer
{
public:
    lrs_sensor_streamer() = default;
    lrs_sensor_streamer( rs2::sensor rs2_sensor,
                    rs2::stream_profile stream_profile,
                    std::function< void( const std::string & stream_name, uint8_t * data, int size ) > cb )
        : _rs2_sensor( rs2_sensor )
        , _stream_profile( stream_profile )
        , _frame_callback(std::move( cb ))
    {
        _rs2_sensor.open( _stream_profile );  // TODO open required profile!
        _rs2_sensor.start( [&]( rs2::frame f ) 
            {
                _frame_callback( _stream_profile.stream_name(), (uint8_t *)f.get_data(), f.get_data_size() );
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
    std::function< void( const std::string& stream_name, uint8_t* data, int size ) > _frame_callback;
};

lrs_device_manager::lrs_device_manager( rs2::device dev )
    : _rs_dev( dev )
{
    _device_sn = _rs_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    std::cout << "LRS device manager for device: " << _device_sn << " created" << std::endl;
}


lrs_device_manager::~lrs_device_manager()
{
    stop_all_streams();
    std::cout << "LRS device manager for device: " << _device_sn << " deleted" << std::endl;
}

void lrs_device_manager::start_stream( rs2::stream_profile sp,
                                       std::function< void( const std::string&, uint8_t*, int ) > cb )
{
    switch( sp.stream_type() )
    {
    case RS2_STREAM_COLOR: {
        auto cs = _rs_dev.first< rs2::color_sensor>();
        stream_to_rs2_sensor[RS2_STREAM_COLOR] = std::make_shared<lrs_device_manager::lrs_sensor_streamer>( cs, sp, cb );
    }
    break;
    case RS2_STREAM_DEPTH: {
        auto ds = _rs_dev.first< rs2::depth_sensor  >();
        stream_to_rs2_sensor[RS2_STREAM_DEPTH]= std::make_shared<lrs_device_manager::lrs_sensor_streamer>( ds, sp, cb );
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

void lrs_device_manager::stop_stream( rs2_stream stream )
{
    if( stream_to_rs2_sensor.find( stream ) == stream_to_rs2_sensor.end() )
    {
        std::cerr << "Cannot stop stream:" << rs2_stream_to_string(stream) << " as it is not streaming" << std::endl;
        return;
    }
    switch( stream )
    {
    case RS2_STREAM_COLOR: {
        stream_to_rs2_sensor.erase( RS2_STREAM_COLOR );
    }
    break;
    case RS2_STREAM_DEPTH: {
        stream_to_rs2_sensor.erase( RS2_STREAM_DEPTH );
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
                                  + std::string( rs2_stream_to_string( stream ) ) );
    }
}

void lrs_device_manager::stop_all_streams()
{
    stream_to_rs2_sensor.clear();
}
