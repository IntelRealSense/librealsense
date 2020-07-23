// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp


#ifndef BUILD_SHARED_LIBS
#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP
#endif


// We have our own main
#define NO_CATCH_CONFIG_MAIN
//#define CATCH_CONFIG_RUNNER

#include "d2rgb-common.h"

//INITIALIZE_EASYLOGGINGPP


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

struct old_algo_calib
{
    algo::matrix_3x3 rot;
    algo::translation trans;
    double __fx, __fy, __ppx, __ppy;  // not in new calib!
    algo::k_matrix k_mat;
    int           width, height;
    rs2_distortion model;
    double         coeffs[5];

    operator algo::calib() const
    {
        algo::calib c;
        c.rot = rot;
        c.trans = trans;
        c.k_mat = k_mat;
        c.width = width;
        c.height = height;
        c.model = model;
        for( auto x = 0; x < 5; ++x )
            c.coeffs[x] = coeffs[x];
        return c;
    }
};

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
            if( !strcmp( dir, "--version" ) )
            {
                // The build number is only available within Jenkins and so we have to hard-
                // code it ><
                std::cout << RS2_API_VERSION_STR << ".1973" << std::endl;
                continue;
            }
            std::cout << "Processing: " << dir << " ..." << std::endl;

            algo::calib calibration;
            try
            {
                read_binary_file( dir, "rgb.calib", &calibration );
            }
            catch( std::exception const & e )
            {
                std::cout << "!! failed: " << e.what() << std::endl;
                old_algo_calib old_calibration;
                read_binary_file( dir, "rgb.calib", &old_calibration );
                calibration = old_calibration;
            }

            camera_params camera;
            camera.rgb = calibration.get_intrinsics();
            camera.extrinsics = calibration.get_extrinsics();
            algo::rs2_intrinsics_double d_intr;  // intrinsics written in double!
            read_binary_file( dir, "depth.intrinsics", &d_intr );
            camera.z = d_intr;
            read_binary_file( dir, "depth.units", &camera.z_units );
            read_binary_file( dir, "cal.info", &camera.cal_info );
            read_binary_file( dir, "cal.registers", &camera.cal_regs );
            read_binary_file( dir, "dsm.params", &camera.dsm_params );

            algo::optimizer cal;
            init_algo( cal, dir, "\\rgb.raw", "\\rgb_prev.raw", "\\ir.raw", "\\depth.raw", camera );

            std::string status;

            TRACE( "\n___\nis_scene_valid" );
            if( !cal.is_scene_valid() )
            {
                TRACE("NOT VALID\n");
                status += "SCENE_INVALID ";
            }

            TRACE( "\n___\noptimize" );
            size_t n_iteration = cal.optimize(
                []( algo::data_collect const & data )
                {
                } );

            TRACE( "\n___\nis_valid_results" );
            if( !cal.is_valid_results() )
            {
                TRACE("NOT VALID\n");
                status += "BAD_RESULT";
            }
            else
            {
                status += "SUCCESSFUL";
            }
            TRACE( "\n___\nRESULTS:  (" << RS2_API_VERSION_STR << " build 1973)" );

            auto intr = cal.get_calibration().get_intrinsics();
            auto extr = cal.get_calibration().get_extrinsics();
            AC_LOG( DEBUG, AC_D_PREC
                << "intr[ "
                << intr.width << "x" << intr.height
                << "  ppx: " << intr.ppx << ", ppy: " << intr.ppy << ", fx: " << intr.fx
                << ", fy: " << intr.fy << ", model: " << int( intr.model ) << " coeffs["
                << intr.coeffs[0] << ", " << intr.coeffs[1] << ", " << intr.coeffs[2]
                << ", " << intr.coeffs[3] << ", " << intr.coeffs[4] << "] ]" );
            AC_LOG( DEBUG, AC_D_PREC << "extr" << (rs2_extrinsics) extr );
            AC_LOG( DEBUG, AC_D_PREC << "dsm" << cal.get_dsm_params() );

            TRACE( "\n___\nVS:" );
            AC_LOG( DEBUG, AC_D_PREC << "dsm" << camera.dsm_params );
         
            TRACE( "\n___\nSTATUS: " + status );
        }
        catch( std::exception const & e )
        {
            std::cerr << "\n___\ncaught exception: " << e.what() << std::endl;
            ok = false;
        }
        catch( ... )
        {
            std::cerr << "\n___\ncaught unknown exception!" << std::endl;
            ok = false;
        }
    }
    
    return ! ok;
}

