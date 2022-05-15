// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <algorithm>
#include <iostream>
#include "lrs-device-manager.h"

using namespace tools;


lrs_device_manager::lrs_device_manager( rs2::device & dev )
    : _rs_dev( dev )
{
    _device_sn = _rs_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    std::cout << "LRS device manager for device: " << _device_sn << " created" << std::endl;
}


lrs_device_manager::~lrs_device_manager()
{
    std::cout << "LRS device manager for device: " << _device_sn << " deleted" << std::endl;
}

void lrs_device_manager::start_stream( stream_type stream,
                                       std::function< void( const std::string&, uint8_t* ) > cb )
{
    switch( stream )
    {
    case stream_type::DEPTH: {
        auto ds = _rs_dev.first< rs2::depth_sensor >();
        stream_to_rs2_sensor["DEPTH"] = std::make_shared<sensor_wrapper>( ds, "DEPTH", cb );
    }
    break;
    case stream_type::RGB: {
        auto cs = _rs_dev.first< rs2::color_sensor >();
        stream_to_rs2_sensor["RGB"]= std::make_shared<sensor_wrapper>( cs, "RGB", cb );
    }
    break;
    default:
        throw std::runtime_error( "start_stream failed: unsupported stream: "
                                  + std::to_string( static_cast< int >( stream ) ) );
    }
}

void lrs_device_manager::stop_stream( stream_type stream )
{
    switch( stream )
    {
    case stream_type::DEPTH: {
        stream_to_rs2_sensor.erase( "DEPTH" );
    }
    break;
    case stream_type::RGB: {
        stream_to_rs2_sensor.erase( "RGB" );
    }
    break;
    default:
        throw std::runtime_error( "stop_stream failed: unsupported stream: "
                                  + std::to_string(static_cast<int>(stream)));
    }
}
