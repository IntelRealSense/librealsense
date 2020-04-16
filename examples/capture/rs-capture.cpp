// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include "../../src/algo/depth-to-rgb-calibration.h"

namespace algo = librealsense::algo::depth_to_rgb_calibration;

#define AC_LOG_PREFIX "AC1: "
//#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
#define AC_LOG(TYPE,MSG) std::cout << (std::string)( librealsense::to_string() << "-" << #TYPE [0] << "- " << MSG ) << std::endl


// Our own derivation of depth_to_rgb_calibration that accepts files on disk
class depth_to_rgb_calibration
{
    // input/output
    rs2_extrinsics _extr;
    rs2_intrinsics _intr;

    algo::optimizer _algo;

public:
    depth_to_rgb_calibration(
        std::string const & z_file,
        std::string const & ir_file,
        std::string const & yuy_file,
        std::string const & prev_yuy_file,
        rs2_intrinsics const & z_intrinsics, float z_units,
        rs2_intrinsics const & yuy_intrinsics,
        rs2_extrinsics const & extrinsics
    );

    rs2_extrinsics const & get_extrinsics() const { return _extr; }
    rs2_intrinsics const & get_intrinsics() const { return _intr; }

    rs2_calibration_status optimize();

private:
    template<class T>
    std::vector<T> get_image( std::string const & file, size_t width, size_t height )
    {
        std::ifstream f;
        f.open( file, std::ios::binary );
        if( !f.good() )
            throw "invalid file";
        std::vector<T> data( width * height );
        f.read( (char*)data.data(), width * height * sizeof( T ) );
        return data;
    }



    void debug_calibration( char const * prefix );
};


depth_to_rgb_calibration::depth_to_rgb_calibration(
    std::string const & z_file,
    std::string const & ir_file,
    std::string const & yuy_file,
    std::string const & prev_yuy_file,
    rs2_intrinsics const & z_intrinsics, float const z_units,
    rs2_intrinsics const & yuy_intrinsics,
    rs2_extrinsics const & extrinsics
)
    : _intr( yuy_intrinsics )
    , _extr( extrinsics )
{
    debug_calibration( "old" );

    AC_LOG( DEBUG, "... setting yuy data" );
    _algo.set_yuy_data(
        get_image< algo::yuy_t >( yuy_file, yuy_intrinsics.width, yuy_intrinsics.height ),
        get_image< algo::yuy_t >( prev_yuy_file, yuy_intrinsics.width, yuy_intrinsics.height ),
        yuy_intrinsics.width, yuy_intrinsics.height
    );

    AC_LOG( DEBUG, "... setting ir data" );
    _algo.set_ir_data(
        get_image< algo::ir_t >( ir_file, z_intrinsics.width, z_intrinsics.height ),
        z_intrinsics.width, z_intrinsics.height
    );

    AC_LOG( DEBUG, "... setting z data" );
    _algo.set_z_data(
        get_image< algo::z_t >( z_file, z_intrinsics.width, z_intrinsics.height ),
        z_intrinsics, z_units
    );

    AC_LOG( DEBUG, "... ready" );
}


rs2_calibration_status depth_to_rgb_calibration::optimize()
{
    try
    {
        AC_LOG( DEBUG, "... checking scene validity" );
        if( !_algo.is_scene_valid() )
        {
            AC_LOG( ERROR, "Calibration scene was found invalid!" );
            return RS2_CALIBRATION_SCENE_INVALID;
        }

        AC_LOG( DEBUG, "... optimizing" );
        auto n_iterations = _algo.optimize( algo::calib( _intr, _extr ));
        if( !n_iterations )
        {
            //AC_LOG( INFO, "Calibration not necessary; nothing done" );
            return RS2_CALIBRATION_NOT_NEEDED;
        }
        AC_LOG( DEBUG, "... " << n_iterations << " iterations" );

        AC_LOG( DEBUG, "... checking result validity" );
        if( !_algo.is_valid_results() )
            return RS2_CALIBRATION_BAD_RESULT;

        _intr = _algo.get_calibration().get_intrinsics();
        _extr = _algo.get_calibration().get_extrinsics();
        debug_calibration( "new" );

        return RS2_CALIBRATION_SUCCESSFUL;
    }
    catch( std::exception const & e )
    {
        AC_LOG( ERROR, "Calibration failed: " << e.what() );
        return RS2_CALIBRATION_FAILED;
    }
}


void depth_to_rgb_calibration::debug_calibration( char const * prefix )
{
    AC_LOG( DEBUG, prefix << " intr: "
        << _intr.width << "x" << _intr.height
        << std::setprecision( 12 )
        << ", ppx: " << _intr.ppx
        << ", ppy: " << _intr.ppy
        << ", fx: " << _intr.fx
        << ", fy: " << _intr.fy
        << ", model: " << int( _intr.model )
        << ", coeffs: ["
        << _intr.coeffs[0] << ", " << _intr.coeffs[1] << ", " << _intr.coeffs[2] << ", " << _intr.coeffs[3] << ", " << _intr.coeffs[4]
        << "]" );
    AC_LOG( DEBUG, prefix << " extr:"
        << std::setprecision( 12 )
        << " r["
        << _extr.rotation[0] << ", " << _extr.rotation[1] << ", " << _extr.rotation[2] << ", " << _extr.rotation[3] << ", " << _extr.rotation[4] << ", "
        << _extr.rotation[5] << ", " << _extr.rotation[6] << ", " << _extr.rotation[7] << ", " << _extr.rotation[8]
        << "]  t["
        << _extr.translation[0] << ", " << _extr.translation[1] << ", " << _extr.translation[2]
        << "]" );
}


// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2_intrinsics depth_intrinsics =
    {
        1024, 768,
        561.812500000000, 397.027343750000,
        728.710937500000,729.546875000000,
        RS2_DISTORTION_NONE, {0, 0, 0, 0, 0}
    };
    float depth_units = 0.25f;
    rs2_intrinsics rgb_intrinsics =
    {
        1920, 1080,
        977.363525390625, 554.286499023438,
        1369.55432128906,1368.58984375000,
        RS2_DISTORTION_BROWN_CONRADY,
        {0.16035768, -0.55488062, -0.00091601571, 0.0018076907,   0.50560439}
    };
    rs2_extrinsics extrinsics =
    {
        { 0.999360322952271, -0.0256289057433605, -0.0249423906207085,
          0.0252323877066374, 0.999552190303803,  -0.0160843506455421,
          0.0253434460610151, 0.0154447052627802,  0.999559462070465 },
        { -1.02322638034821, 13.7332067489624, 1.02426385879517 }
    };

    const std::string root_dir = "C:/work/autocal/LongRange 768X1024 (RGB 1920X1080)/";
    depth_to_rgb_calibration cal(
        root_dir + "2/Z_GrayScale_1024x768_00.00.26.7119_F9440687_0000.raw",
        root_dir + "2/I_GrayScale_1024x768_00.00.26.7119_F9440687_0000.raw",
        root_dir + "2/YUY2_YUY2_1920x1080_00.00.26.6355_F9440687_0000.raw",
        root_dir + "2/YUY2_YUY2_1920x1080_00.00.26.6355_F9440687_0000.raw",
        depth_intrinsics, depth_units, rgb_intrinsics,
        extrinsics
    );
    auto status = cal.optimize();
    AC_LOG( INFO, "Auto calibration finished, status= " << rs2_calibration_status_to_string( status ));
    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
