//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "depth-to-rgb-calibration.h"
#include <librealsense2/rs.hpp>
#include "core/streaming.h"
#include "calibrated-sensor.h"
#include "context.h"
#include "api.h"  // VALIDATE_INTERFACE_NO_THROW
#include "algo/depth-to-rgb-calibration/debug.h"

#ifndef _WIN32
#include <sys/stat.h>  // mkdir
#endif


using namespace librealsense;
namespace impl = librealsense::algo::depth_to_rgb_calibration;

depth_to_rgb_calibration::depth_to_rgb_calibration(
    rs2::frame depth,
    rs2::frame ir,
    rs2::frame yuy,
    rs2::frame prev_yuy,
    algo::depth_to_rgb_calibration::algo_calibration_info const & cal_info,
    algo::depth_to_rgb_calibration::algo_calibration_registers const & cal_regs
)
    : _intr( yuy.get_profile().as< rs2::video_stream_profile >().get_intrinsics() )
    , _extr(to_raw_extrinsics( depth.get_profile().get_extrinsics_to( yuy.get_profile() )))
    , _from( depth.get_profile().get()->profile )
    , _to( yuy.get_profile().get()->profile )
{
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
        z_profile.get_intrinsics(), _dsm_params, cal_info, cal_regs,
        depth.as< rs2::depth_frame >().get_units() * 1000.f   // same scaling as for extrinsics!
    );

    debug_calibration( "old" );

    // If the user has this env var defined, then we write out logs and frames to it
    // NOTE: The var should end with a directory separator \ or /
    auto dir_ = getenv( "RS2_DEBUG_DIR" );
    if( dir_ )
    {
        std::string dir( dir_ );
        dir += std::to_string( depth.get_frame_number() );
        
#ifdef _WIN32
        dir += "\\";
        auto status = _mkdir( dir.c_str() );
#else
        dir += "/";
        auto status = mkdir( dir.c_str(), 0700 );
#endif
        if( status == 0 )
            _algo.write_data_to( dir );
        else
            AC_LOG( WARNING, "Failed (" << status << ") to write AC frame data to: " << dir );
    }
}


rs2_calibration_status depth_to_rgb_calibration::optimize(
    std::function<void( rs2_calibration_status )> call_back
)
{
#define DISABLE_RS2_CALIBRATION_CHECKS "RS2_AC_DISABLE_RETRIES"

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
            if( getenv( "RS2_AC_INVALID_SCENE_FAIL" ) )
            {
                // Here we don't want a retry, but we also do not want the calibration
                // to possibly be successful -- fail it
                AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on but so is RS2_AC_INVALID_SCENE_FAIL: fail!" );
                return RS2_CALIBRATION_FAILED;
            }
            AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on; continuing" );
        }

        AC_LOG( DEBUG, "... optimizing" );
        _algo.optimize();

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
            if( !getenv( "RS2_AC_FORCE_BAD_RESULT" ) )
            {
                // This is mostly for validation use, where we don't want the retries and instead want
                // to fail on bad results (we don't want to write bad results to the camera!)
                AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on; no retries" );
                return RS2_CALIBRATION_FAILED;
            }
            // Allow forcing of results... be careful! This may damage the camera in AC2!
            AC_LOG( DEBUG, DISABLE_RS2_CALIBRATION_CHECKS << " is on but so is RS2_AC_FORCE_BAD_RESULT: results will be used!" );
        }

        // AC_LOG( INFO, "Calibration finished; original cost= " << original_cost << "  optimized
        // cost= " << params_curr.cost );

        AC_LOG( DEBUG, "... optimization successful!" );
        _intr = _algo.get_calibration().get_intrinsics();
        _intr.model = RS2_DISTORTION_INVERSE_BROWN_CONRADY; //restore LRS model 
        _extr = from_raw_extrinsics( _algo.get_calibration().get_extrinsics() );
        _dsm_params = _algo.get_dsm_params();
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
    AC_LOG( DEBUG, AC_F_PREC << prefix << _intr );
    AC_LOG( DEBUG, AC_F_PREC << prefix << " extr" << _extr );
    AC_LOG( DEBUG, AC_F_PREC << prefix << " dsm" << _dsm_params );
}

