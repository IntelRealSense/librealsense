//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "depth-to-rgb-calibration.h"
#include <librealsense2/rs.hpp>
#include "core/streaming.h"
#include "calibrated-sensor.h"
#include "context.h"
#include "api.h"  // VALIDATE_INTERFACE_NO_THROW
#include "algo/depth-to-rgb-calibration/debug.h"


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

    auto si = ((frame_interface *) depth.get() )->get_sensor();
    auto cs = VALIDATE_INTERFACE_NO_THROW( si, librealsense::calibrated_sensor );
    if( !cs )
    {
        // We can only calibrate depth sensors that supply this interface!
        throw librealsense::not_implemented_exception( "the depth frame supplied is not from a calibrated_sensor" );
    }
    _dsm_params = cs->get_dsm_params();

    AC_LOG( DEBUG, "... setting z data" );
    auto z_profile = depth.get_profile().as< rs2::video_stream_profile >();
    auto z_data = (impl::z_t const *) depth.get_data();
    _algo.set_z_data(
        std::vector< impl::z_t >( z_data, z_data + depth.get_data_size() / sizeof( impl::z_t ) ),
        z_profile.get_intrinsics(), _dsm_params,
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


rs2_calibration_status depth_to_rgb_calibration::optimize(
    std::function<void( rs2_calibration_status )> call_back
)
{
#define DISABLE_RS2_CALIBRATION_CHECKS "DISABLE_RS2_CALIBRATION_CHECKS"

    try
    {
        AC_LOG( DEBUG, "... checking scene validity" );
        if( !_algo.is_scene_valid() )
        {
            AC_LOG( ERROR, "Calibration scene was found invalid!" );
            call_back( RS2_CALIBRATION_SCENE_INVALID );
            if( !getenv( DISABLE_RS2_CALIBRATION_CHECKS ) )
            {
                // Default behavior is to stop AC and trigger a retry
                return RS2_CALIBRATION_RETRY;
            }
            AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on; continuing" );
        }

        AC_LOG( DEBUG, "... optimizing" );
        auto n_iterations = _algo.optimize();
        if( !n_iterations )
        {
            // AC_LOG( INFO, "Calibration not necessary; nothing done" );
            return RS2_CALIBRATION_NOT_NEEDED;
        }

        AC_LOG( DEBUG, "... checking result validity" );
        if( !_algo.is_valid_results() )
        {
            // Error would have printed inside
            call_back( RS2_CALIBRATION_BAD_RESULT );
            if( !getenv( DISABLE_RS2_CALIBRATION_CHECKS ) )
            {
                // Default behavior is to stop and trigger a retry
                AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is off; will retry if possible" );
                return RS2_CALIBRATION_RETRY;
            }
            if( !getenv( "FORCE_RS2_CALIBRATION_RESULTS" ) )
            {
                // This is mostly for validation use, where we don't want the retries and instead want
                // to fail on bad results (we don't want to write bad results to the camera!)
                AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on; no retries" );
                return RS2_CALIBRATION_FAILED;
            }
            // Allow forcing of results... be careful! This may damage the camera in AC2!
            AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on but so is FORCE_RS2_CALIBRATION_RESULTS: results will be used!" );
        }

        // AC_LOG( INFO, "Calibration finished; original cost= " << original_cost << "  optimized
        // cost= " << params_curr.cost );

        AC_LOG( DEBUG, "... optimization successful!" );
        _intr = _algo.get_calibration().get_intrinsics();
        _extr = fix_extrinsics( _algo.get_calibration().get_extrinsics(), 0.001f );
        debug_calibration( "new" );
        return RS2_CALIBRATION_SUCCESSFUL;
    }
    catch( std::exception const & e )
    {
        AC_LOG( ERROR, "Calibration threw error: " << e.what() );
        return RS2_CALIBRATION_FAILED;
    }
}


void depth_to_rgb_calibration::debug_calibration( char const * prefix )
{
    AC_LOG( DEBUG, std::setprecision( std::numeric_limits< float >::max_digits10 )
                       << prefix << " intr[ "
                       << _intr.width << "x" << _intr.height
                       << "  ppx: " << _intr.ppx << ", ppy: " << _intr.ppy << ", fx: " << _intr.fx
                       << ", fy: " << _intr.fy << ", model: " << int( _intr.model ) << " coeffs["
                       << _intr.coeffs[0] << ", " << _intr.coeffs[1] << ", " << _intr.coeffs[2]
                       << ", " << _intr.coeffs[3] << ", " << _intr.coeffs[4] << "] ]" );
    AC_LOG( DEBUG, std::setprecision( std::numeric_limits< float >::max_digits10 )
                       << prefix << " extr" << _extr );
    AC_LOG( DEBUG, std::setprecision( std::numeric_limits< float >::max_digits10 )
                       << prefix << " dsm" << _dsm_params );
}

