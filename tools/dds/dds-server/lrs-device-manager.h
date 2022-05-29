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
                    std::string stream_name,
                    std::function< void( const std::string & stream_name, uint8_t * data ) > cb )
        : _rs2_sensor( rs2_sensor )
        , _stream_name( stream_name )
        , _frame_callback(std::move( cb ))
    {
        _rs2_sensor.open( _rs2_sensor.get_stream_profiles()[0] );  // TODO open required profile!
        _rs2_sensor.start( [&]( rs2::frame f ) 
            {
                _frame_callback( _stream_name, (uint8_t *)f.get_data() );
            } );
        std::cout << _stream_name << " stream started"  << std::endl;
    }
    ~sensor_wrapper()
    {
        _rs2_sensor.stop();
        _rs2_sensor.close();
        std::cout << _stream_name << " stream stopped"  << std::endl;
    };

private:
    rs2::sensor _rs2_sensor;
    std::string _stream_name;
    std::function< void( const std::string& stream_name, uint8_t* data ) > _frame_callback;
};

enum class stream_type{DEPTH, RGB}; // TODO - add additional streams support

// This class is in charge of handling a RS device: streaming, control..
class lrs_device_manager
{
public:
    
    lrs_device_manager( rs2::device dev );
    ~lrs_device_manager();
    void start_stream( stream_type stream, std::function< void( const std::string&, uint8_t* ) > cb );
    void stop_stream( stream_type stream );

private:
    rs2::device _rs_dev;
    std::string _device_sn;
    std::unordered_map<std::string, std::shared_ptr<sensor_wrapper>> stream_to_rs2_sensor;
};  // class lrs_device_manager
}  // namespace tools
