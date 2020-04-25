// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp

// We have our own main
#define NO_CATCH_CONFIG_MAIN

#include "../../../src/algo/depth-to-rgb-calibration/optimizer.h"
#include "../algo-common.h"
#include "ac-logger.h"
#include "F9440687.h"

ac_logger LOG_TO_STDOUT;


namespace algo = librealsense::algo::depth_to_rgb_calibration;
using librealsense::to_string;


template< typename T >
void read_binary_file( char const * dir, char const * bin, T * data )
{
    std::string filename = dir;
    filename += "\\";
    filename += bin;
    AC_LOG( DEBUG, "... " << filename );
    std::fstream f = std::fstream( filename, std::ios::in | std::ios::binary );
    if( ! f )
        throw std::runtime_error( "failed to read file:\n" + filename );
    f.seekg( 0, f.end );
    size_t cb = f.tellg();
    f.seekg( 0, f.beg );
    if( cb != sizeof( T ) )
        throw std::runtime_error( to_string()
            << "file size (" << cb << ") does not match data size (" << sizeof(T) << "): " << filename );
    std::vector< T > vec( cb / sizeof( T ));
    f.read( (char*) data, cb );
    f.close();
}

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
    f.read( (char*) data.data(), width * height * sizeof( T ));
    return data;
}

void init_algo(algo::optimizer & cal,
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

int main( int argc, char * argv[] )
{
    bool ok = true;
    // Each of the arguments is the path to a directory to simulate
    // We skip argv[0] which is the path to the executable
    // We don't complain if no arguments -- that's how we'll run as part of unit-testing
    for( int i = 1; i < argc; ++i )
    {
        try
        {
            char const * dir = argv[i];
            std::cout << "Processing: " << dir << " ..." << std::endl;

            algo::calib calibration;
            read_binary_file( dir, "rgb.calib", &calibration );

            camera_info camera;
            camera.rgb = calibration.get_intrinsics();
            camera.extrinsics = calibration.get_extrinsics();
            algo::rs2_intrinsics_double d_intr;  // intrinsics written in double!
            read_binary_file( dir, "depth.intrinsics", &d_intr );
            camera.z = d_intr;
            read_binary_file( dir, "depth.units", &camera.z_units );

            algo::optimizer cal;
            init_algo( cal, dir, "\\rgb.raw", "\\rgb_prev.raw", "\\ir.raw", "\\depth.raw", camera );

            TRACE( "is_scene_valid" );
            if( !cal.is_scene_valid() )
            {

            }

            TRACE( "optimize" );
            size_t n_iteration = cal.optimize(
                []( algo::iteration_data_collect const & data )
                {
                } );

            TRACE( "is_valid_results" );
            if( !cal.is_valid_results() )
            {

            }
            TRACE( "done!" );
        }
        catch( std::exception const & e )
        {
            std::cerr << "caught exception: " << e.what() << std::endl;
            ok = false;
        }
        catch( ... )
        {
            std::cerr << "caught unknown exception!" << std::endl;
            ok = false;
        }
    }
    
    return ok;
}

