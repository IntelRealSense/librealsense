// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef TESTS_H
#define TESTS_H

#include "../include/librealsense2/rs.hpp"

#define CATCH_CONFIG_MAIN

#include "catch.h"

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

inline rs2::device_list find_device_by_product_line_or_exit( int product )
{
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices( product );
    size_t device_count = devices_list.size();
    if( device_count == 0 )
    {
        std::cout << "No L500 device connected ";
        exit( 0 );
    }

    return devices_list;
}

inline rs2::depth_sensor get_depth_sensor( rs2::device dev )
{
    auto sensors = dev.query_sensors();

    auto it = std::find_if( sensors.begin(), sensors.end(), []( rs2::sensor s ) {
        return s.is< rs2::depth_sensor >();
    } );

    return *it;
}

inline void remove_all_streams_arrived( rs2::frame f,
                                        std::vector< rs2::stream_profile > & stream_to_arrive )
{
    auto remove_stream = [&]() {
        auto it = std::remove_if( stream_to_arrive.begin(),
                                  stream_to_arrive.end(),
                                  [&]( rs2::stream_profile s ) {
                                      return s.stream_type() == f.get_profile().stream_type();
                                  } );


        if( it != stream_to_arrive.end() )
            stream_to_arrive.erase( it );
    };

    if( f.is< rs2::frameset >() )
    {
        auto set = f.as< rs2::frameset >();
        set.foreach_rs( [&]( rs2::frame fr ) { remove_stream(); } );
    }
    else
    {
        remove_stream();
    }
}

inline std::vector< uint32_t > split( const std::string & s, char delim )
{
    std::stringstream ss( s );
    std::string item;
    std::vector< uint32_t > tokens;
    while( std::getline( ss, item, delim ) )
    {
        tokens.push_back( std::stoi( item, nullptr ) );
    }
    return tokens;
}

inline bool is_fw_version_newer( rs2::sensor & subdevice, const uint32_t other_fw[4] )
{
    std::string fw_version_str;
    REQUIRE_NOTHROW( fw_version_str = subdevice.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) );
    auto fw = split( fw_version_str, '.' );
    if( fw[0] > other_fw[0] )
        return true;
    if( fw[0] == other_fw[0] && fw[1] > other_fw[1] )
        return true;
    if( fw[0] == other_fw[0] && fw[1] == other_fw[1] && fw[2] > other_fw[2] )
        return true;
    if( fw[0] == other_fw[0] && fw[1] == other_fw[1] && fw[2] == other_fw[2]
        && fw[3] > other_fw[3] )
        return true;
    if( fw[0] == other_fw[0] && fw[1] == other_fw[1] && fw[2] == other_fw[2]
        && fw[3] == other_fw[3] )
        return true;
    return false;
}
#endif
