// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../test.h"
#include "librealsense2/rs.hpp"

using namespace rs2;

static const char L515_DEVICE[] = "L515";

inline rs2::device_list find_devices_by_product_line_or_exit( int product )
{
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices( product );
    if( devices_list.size() == 0 )
    {
        std::cout << "No device of the " << product << " product line was found; skipping test"
                  << std::endl;
        exit( 0 );
    }

    return devices_list;
}

// This function returns the first device found that it's name contains the given string,
// if there is no such device it will exit the test.
// Can get as input a full device name or short like "L515/D435..."
inline rs2::device find_device_by_name_or_exit( const std::string & dev_name )
{
    rs2::context ctx;
    std::vector< rs2::device > devices_list = ctx.query_devices();

    auto dev_iter
        = std::find_if( devices_list.begin(), devices_list.end(), [dev_name]( rs2::device dev ) {
              return dev.supports( RS2_CAMERA_INFO_NAME )
                       ? std::string( dev.get_info( RS2_CAMERA_INFO_NAME ) ).find( dev_name )
                             != std::string::npos
                       : false;
          } );

    if( dev_iter != devices_list.end() )
    {
        return *dev_iter;
    }

    std::cout << "No " << dev_name << " device was found; skipping test" << std::endl;
    exit( 0 );
}

// Return the first device that supports the input option or exits.
// Can get as input a full device name or short like "L515/D435..."
rs2::depth_sensor find_first_supported_depth_sensor_or_exit(const std::string & dev_name,
    rs2_option opt)
{
    auto dev = find_device_by_name_or_exit(dev_name);

    auto ds = dev.first< rs2::depth_sensor >();
    if (!ds.supports(opt))
        exit(0);

    return ds;
}

// Remove the frame's stream (or streams if a frameset) from the list of streams we expect to arrive
// If any stream is unexpected, it is ignored
inline void remove_all_streams_arrived( rs2::frame f,
                                        std::vector< rs2::stream_profile > & expected_streams )
{
    auto remove_stream = [&]() {
        auto it = std::remove_if( expected_streams.begin(),
                                  expected_streams.end(),
                                  [&]( rs2::stream_profile s ) {
                                      return s.stream_type() == f.get_profile().stream_type();
                                  } );


        if( it != expected_streams.end() )
            expected_streams.erase( it );
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

inline stream_profile find_default_depth_profile( rs2::depth_sensor depth_sens )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto depth_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), []( stream_profile sp ) {
              return sp.is_default() && sp.stream_type() == RS2_STREAM_DEPTH;
          } );

    REQUIRE( depth_profile != stream_profiles.end() );
    return *depth_profile;
}

inline stream_profile find_default_ir_profile( rs2::depth_sensor depth_sens )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto ir_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), []( stream_profile sp ) {
              return sp.is_default() && sp.stream_type() == RS2_STREAM_INFRARED;
          } );

    REQUIRE( ir_profile != stream_profiles.end() );
    return *ir_profile;
}

inline stream_profile find_confidence_corresponding_to_depth( rs2::depth_sensor depth_sens,
                                                       stream_profile depth_profile )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto confidence_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), [&]( stream_profile sp ) {
              return sp.stream_type() == RS2_STREAM_CONFIDENCE
                  && sp.as< rs2::video_stream_profile >().width()
                         == depth_profile.as< rs2::video_stream_profile >().width()
                  && sp.as< rs2::video_stream_profile >().height()
                         == depth_profile.as< rs2::video_stream_profile >().height();
          } );

    REQUIRE( confidence_profile != stream_profiles.end() );
    return *confidence_profile;
}


bool get_profile_by_stream_parameters( rs2::depth_sensor ds,
                                       rs2_stream stream,
                                       rs2_format format,
                                       int width,
                                       int height,
                                       rs2::stream_profile & required_profile )
{
    auto profiles = ds.get_stream_profiles();
    auto vga_depth_profile
        = std::find_if(profiles.begin(), profiles.end(), [=](rs2::stream_profile sp) {
        auto vp = sp.as< rs2::video_stream_profile >();
        return ((sp.stream_type() == stream) && (sp.format() == format)
            && (vp.width() == width) && (vp.height() == height));
    });

    if (vga_depth_profile != profiles.end())
    {
        required_profile = *vga_depth_profile;
        return true;
    }

    return false;
}


