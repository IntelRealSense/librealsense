//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "depth-to-rgb-calibration.h"
#include <librealsense2/rs.hpp>
#include "core/streaming.h"
#include "context.h"

#define AC_LOG_PREFIX "AC1: "
#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) std::cout << (std::string)( to_string() << "-" << #TYPE [0] << "- " << MSG ) << std::endl


using namespace librealsense;
namespace impl = librealsense::algo::depth_to_rgb_calibration;


static rs2_extrinsics fix_extrinsics( rs2_extrinsics extr, float by )
{
    // The extrinsics we get are based in meters, and AC algo is based in millimeters
    // NOTE that the scaling here needs to be accompanied by the same scaling of the depth
    // units!
    extr.translation[0] *= by;
    extr.translation[1] *= by;
    extr.translation[2] *= by;
    // This transposing is absolutely mandatory because our internal algorithms are
    // written with a transposed matrix in mind! (see rs2_transform_point_to_point)
    // This is the opposite transpose to the one we do with the extrinsics we get
    // from the camera...
    std::swap( extr.rotation[1], extr.rotation[3] );
    std::swap( extr.rotation[2], extr.rotation[6] );
    std::swap( extr.rotation[5], extr.rotation[7] );
    return extr;
}


depth_to_rgb_calibration::depth_to_rgb_calibration(
    rs2::frame depth,
    rs2::frame ir,
    rs2::frame yuy,
    rs2::frame prev_yuy
)
    : _intr( yuy.get_profile().as< rs2::video_stream_profile >().get_intrinsics() )
    , _extr( fix_extrinsics( depth.get_profile().get_extrinsics_to( yuy.get_profile() ), 1000 ))
    , _from( depth.get_profile().get()->profile )
    , _to( yuy.get_profile().get()->profile )
{
    debug_calibration( "old" );

    AC_LOG( DEBUG, "... setting yuy data" );
    auto color_profile = yuy.get_profile().as< rs2::video_stream_profile >();
    auto yuy_data = (impl::yuy_t const *) yuy.get_data();
    auto prev_yuy_data = (impl::yuy_t const *) prev_yuy.get_data();
    impl::calib calibration( _intr, _extr );
    _algo.set_yuy_data(
        std::vector< impl::yuy_t >( yuy_data, yuy_data + yuy.get_data_size() / sizeof( impl::yuy_t )),
        std::vector< impl::yuy_t >( prev_yuy_data, prev_yuy_data + yuy.get_data_size() / sizeof( impl::yuy_t ) ),
        calibration
    );

    AC_LOG( DEBUG, "... setting ir data" );
    auto ir_profile = ir.get_profile().as< rs2::video_stream_profile >();
    auto ir_data = (impl::ir_t const *) ir.get_data();
    _algo.set_ir_data(
        std::vector< impl::ir_t >( ir_data, ir_data + ir.get_data_size() / sizeof( impl::ir_t )),
        ir_profile.width(), ir_profile.height()
    );

    AC_LOG( DEBUG, "... setting z data" );
    auto z_profile = depth.get_profile().as< rs2::video_stream_profile >();
    auto z_data = (impl::z_t const *) depth.get_data();
    _algo.set_z_data(
        std::vector< impl::z_t >( z_data, z_data + depth.get_data_size() / sizeof( impl::z_t ) ),
        z_profile.get_intrinsics(),
        depth.as< rs2::depth_frame >().get_units() * 1000.f   // same scaling as for extrinsics!
    );

    // TODO REMOVE
#ifdef _WIN32
    std::string dir = "C:\\work\\autocal\\data\\";
    dir += to_string() << depth.get_frame_number();
    if( _mkdir( dir.c_str() ) == 0 )
        _algo.write_data_to( dir );
    else
        AC_LOG( WARNING, "Failed to write AC frame data to: " << dir );
#endif
}


rs2_calibration_status depth_to_rgb_calibration::optimize()
{
    try
    {
        AC_LOG( DEBUG, "... checking scene validity" );
        if( !_algo.is_scene_valid() )
        {
            AC_LOG( ERROR, "Calibration scene was found invalid!" );
            //return RS2_CALIBRATION_SCENE_INVALID;
        }

        AC_LOG( DEBUG, "... optimizing" );
        auto n_iterations = _algo.optimize();
        if( !n_iterations )
        {
            //AC_LOG( INFO, "Calibration not necessary; nothing done" );
            return RS2_CALIBRATION_NOT_NEEDED;
        }

        AC_LOG( DEBUG, "... checking result validity" );
        if( !_algo.is_valid_results() )
            ; // return RS2_CALIBRATION_BAD_RESULT;

        //AC_LOG( INFO, "Calibration finished; original cost= " << original_cost << "  optimized cost= " << params_curr.cost );

        _intr = _algo.get_calibration().get_intrinsics();
        _extr = fix_extrinsics( _algo.get_calibration().get_extrinsics(), 0.001f );
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
    AC_LOG( DEBUG, prefix << " intr"
        << ": width: " << _intr.width
        << ", height: " << _intr.height
        << ", ppx: " << _intr.ppx
        << ", ppy: " << _intr.ppy
        << ", fx: " << _intr.fx
        << ", fy: " << _intr.fy
        << ", model: " << int( _intr.model)
        << ", coeffs: ["
        << _intr.coeffs[0] << ", " << _intr.coeffs[1] << ", " << _intr.coeffs[2] << ", " << _intr.coeffs[3] << ", " << _intr.coeffs[4]
        << "]" );
    AC_LOG( DEBUG, prefix << " extr:"
        << " rotation: ["
        << _extr.rotation[0] << ", " << _extr.rotation[1] << ", " << _extr.rotation[2] << ", " << _extr.rotation[3] << ", " << _extr.rotation[4] << ", "
        << _extr.rotation[5] << ", " << _extr.rotation[6] << ", " << _extr.rotation[7] << ", " << _extr.rotation[8]
        << "]  translation: ["
        << _extr.translation[0] << ", " << _extr.translation[1] << ", " << _extr.translation[2]
        << "]" );
}

