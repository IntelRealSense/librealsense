// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "../algo-common.h"
#include "../../../src/algo/depth-to-rgb-calibration/optimizer.h"
#include "ac-logger.h"
#include "../camera-info.h"

ac_logger LOG_TO_STDOUT;


namespace algo = librealsense::algo::depth_to_rgb_calibration;
using librealsense::to_string;


//static char const * root_data_dir = "C:\\work\\autocal\\";
static char const* root_data_dir = "C:\\Users\\nyassin\\Documents\\realsense_all\\debug_scene\\"; // "C:\\Users\\nyassin\\Documents\\realsense_all\\";// "..\\unit-tests\\algo\\depth-to-rgb-calibration\\19.2.20\\";

static char const * const data_dirs[] = {
    "LongRange 768X1024 (RGB 1920X1080)"//"binFiles_avishag"//"F9440687\\Snapshots\\LongRange_D_768x1024_RGB_1920x1080"
};

static size_t const n_data_dirs = sizeof( data_dirs ) / sizeof( data_dirs[0] );


template<class T>
std::vector< T > read_image_file( std::string const & file, size_t width, size_t height )
{
    std::ifstream f;
    f.open( file, std::ios::in | std::ios::binary );
    if( !f.good() )
        throw std::runtime_error( "invalid file: " + file );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb != sizeof( T ) * width * height )
        throw std::runtime_error( to_string()
            << "file size (" << cb << ") does not match expected size (" << sizeof( T ) * width * height << "): " << file );
    std::vector< T > data( width * height );
    f.read( (char*)data.data(), width * height * sizeof( T ) );
    return data;
}

template< typename T >
void dump_vec( std::vector< double > const & cpp, std::vector< T > const & matlab,
    char const * basename,
    size_t width, size_t height
)
{
    std::string filename = basename;
    filename += ".dump";
#if 0
    std::fstream f = std::fstream( filename, std::ios::out );
    if( !f )
        throw std::runtime_error( "failed to write file:\n" + filename );
    for( size_t x = )
        std::vector< T > vec( cb / sizeof( T ) );
    f.read( (char*)vec.data(), cb );
    f.close();
    return vec;
#endif
}

void init_algo( algo::optimizer & cal,
    std::string const & dir,
    char const * yuy,
    char const * yuy_prev,
    char const * ir,
    char const * z,
    camera_info const & camera
)
{
    TRACE( "Loading " << dir << " ..." );

    algo::calib calibration( camera.rgb, camera.extrinsics );

    cal.set_yuy_data(
        read_image_file< algo::yuy_t >( dir + yuy, camera.rgb.width, camera.rgb.height ),
        read_image_file< algo::yuy_t >( dir + yuy_prev, camera.rgb.width, camera.rgb.height ),
        calibration
    );

    cal.set_ir_data(
        read_image_file< algo::ir_t >( dir + ir, camera.z.width, camera.z.height ),
        camera.z.width, camera.z.height
    );

    cal.set_z_data(
        read_image_file< algo::z_t >( dir + z, camera.z.width, camera.z.height ),
        camera.z, camera.z_units
    );
}
