// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API
#include <unordered_map>

namespace tools {

class sensor_wrapper
{
public:
    sensor_wrapper() = default;
    sensor_wrapper( rs2::sensor rs2_sensor,
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
    ~sensor_wrapper()
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

// This class is in charge of handling a RS device: streaming, control..
class lrs_device_manager
{
public:
    
    lrs_device_manager( rs2::device dev );
    ~lrs_device_manager();
    void start_stream( rs2::stream_profile sp, std::function< void( const std::string&, uint8_t*, int ) > cb );
    void stop_stream( rs2_stream stream );
    void stop_all_streams();

private:
    rs2::device _rs_dev;
    std::string _device_sn;
    std::unordered_map<rs2_stream, std::shared_ptr<sensor_wrapper>> stream_to_rs2_sensor;
};  // class lrs_device_manager
}  // namespace tools
