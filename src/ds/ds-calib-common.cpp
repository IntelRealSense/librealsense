// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include <src/ds/ds-calib-common.h>

#include <src/types.h>
#include <src/device.h>
#include <src/auto-calibrated-device.h>
#include <src/librealsense-exception.h>
#include <src/core/sensor-interface.h>
#include <src/core/extension.h>
#include <librealsense2/hpp/rs_processing.hpp>

#include <rsutils/json.h>
#include <rsutils/string/from.h>

#include <array>

namespace librealsense
{
    thermal_compensation_guard::thermal_compensation_guard( auto_calibrated_interface * handle ) : restart_tl(false), snr(nullptr)
    {
        if( Is< librealsense::device >( handle ) )
        {
            try
            {
                snr = &( As< librealsense::device >( handle )->get_sensor( 0 ) ); // Depth sensor is assigned first by design

                if( snr->supports_option( RS2_OPTION_THERMAL_COMPENSATION ) )
                    restart_tl = ( snr->get_option( RS2_OPTION_THERMAL_COMPENSATION ).query() != 0 );

                if( restart_tl )
                {
                    snr->get_option( RS2_OPTION_THERMAL_COMPENSATION ).set( 0.f );
                    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ); // Allow for FW changes to propagate
                }
            }
            catch( ... )
            {
                LOG_WARNING( "Thermal Compensation guard failed to invoke" );
            }
        }
    }

    thermal_compensation_guard::~thermal_compensation_guard()
    {
        try
        {
            if (snr && restart_tl)
                snr->get_option(RS2_OPTION_THERMAL_COMPENSATION).set(1.f);
        }
        catch (...) {
            LOG_WARNING("Thermal Compensation guard failed to complete");
        }
    }

    std::map< std::string, int > ds_calib_common::parse_json( const std::string & json_content )
    {
        auto j = rsutils::json::parse( json_content );

        std::map< std::string, int > values;

        for( auto it = j.begin(); it != j.end(); ++it )
        {
            values[it.key()] = it.value();
        }

        return values;
    }

    void ds_calib_common::update_value_if_exists( const std::map< std::string, int > & jsn, const std::string & key, int & value )
    {
        auto it = jsn.find( key );
        if( it != jsn.end() )
            value = it->second;
    }

    void ds_calib_common::check_params( int speed, int scan_parameter, int data_sampling )
    {
        if( speed < ds_calib_common::SPEED_VERY_FAST || speed > ds_calib_common::SPEED_WHITE_WALL )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'speed' "
                                           << speed << " is out of range (0 - 4)." );
        if( scan_parameter != ds_calib_common::PY_SCAN && scan_parameter != ds_calib_common::RX_SCAN )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'scan parameter' "
                                           << scan_parameter << " is out of range (0 - 1)." );
        if( data_sampling != ds_calib_common::POLLING && data_sampling != ds_calib_common::INTERRUPT )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'data sampling' "
                                           << data_sampling << " is out of range (0 - 1)." );
    }

    void ds_calib_common::check_tare_params( int speed, int scan_parameter, int data_sampling,
                                             int average_step_count, int step_count, int accuracy )
    {
        check_params( speed, scan_parameter, data_sampling );

        if( average_step_count < 1 || average_step_count > 30 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'number of frames to average' "
                                           << average_step_count << " is out of range (1 - 30)." );
        if( step_count < 5 || step_count > 30 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'max iteration steps' "
                                           << step_count << " is out of range (5 - 30)." );
        if( accuracy < ACCURACY_HIGH || accuracy > ACCURACY_LOW )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'subpixel accuracy' " << accuracy
                                           << " is out of range (0 - 3)." );
    }

    void ds_calib_common::check_focal_length_params( int step_count,
                                                     int fy_scan_range,
                                                     int keep_new_value_after_sucessful_scan,
                                                     int interrrupt_data_samling,
                                                     int adjust_both_sides,
                                                     int fl_scan_location,
                                                     int fy_scan_direction,
                                                     int white_wall_mode )
    {
        if( step_count < 8 || step_count > 256 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'step_count' " << step_count
                                           << " is out of range (8 - 256)." );
        if( fy_scan_range < 1 || fy_scan_range > 60000 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'fy_scan_range' " << fy_scan_range
                                           << " is out of range (1 - 60000)." );
        if( keep_new_value_after_sucessful_scan < 0 || keep_new_value_after_sucessful_scan > 1 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'keep_new_value_after_sucessful_scan' "
                                           << keep_new_value_after_sucessful_scan << " is out of range (0 - 1)." );
        if( interrrupt_data_samling < 0 || interrrupt_data_samling > 1 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'interrrupt_data_samling' "
                                           << interrrupt_data_samling << " is out of range (0 - 1)." );
        if( adjust_both_sides < 0 || adjust_both_sides > 1 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'adjust_both_sides' "
                                           << adjust_both_sides << " is out of range (0 - 1)." );
        if( fl_scan_location < 0 || fl_scan_location > 1 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'fl_scan_location' "
                                           << fl_scan_location << " is out of range (0 - 1)." );
        if( fy_scan_direction < 0 || fy_scan_direction > 1 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'fy_scan_direction' "
                                           << fy_scan_direction << " is out of range (0 - 1)." );
        if( white_wall_mode < 0 || white_wall_mode > 1 )
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'white_wall_mode' "
                                           << white_wall_mode << " is out of range (0 - 1)." );
    }

    void ds_calib_common::get_target_rect_info( rs2_frame_queue * frames,
                                                float rect_sides[4],
                                                float & fx,
                                                float & fy,
                                                int progress,
                                                rs2_update_progress_callback_sptr progress_callback )
    {
        rs2_error * e = nullptr;
        int queue_size = rs2_frame_queue_size( frames, &e );
        if( queue_size == 0 )
            throw std::runtime_error( "Extract target rectangle info - no frames in input queue!" );

        int frame_counter = 0;
        bool first_time = true;
        rs2_frame * f = nullptr;
        std::vector< std::array< float, 4 > > rect_sides_arr;
        while( ( frame_counter++ < queue_size ) && rs2_poll_for_frame( frames, &f, &e ) )
        {
            rs2::frame ff( f );
            if( ff.get_data() )
            {
                if( first_time )
                {
                    auto p = ff.get_profile();
                    auto vsp = p.as< rs2::video_stream_profile >();
                    rs2_intrinsics intrin = vsp.get_intrinsics();
                    fx = intrin.fx;
                    fy = intrin.fy;
                    first_time = false;
                }

                std::array< float, 4 > rec_sides_cur{};
                rs2_extract_target_dimensions( f,
                                               RS2_CALIB_TARGET_ROI_RECT_GAUSSIAN_DOT_VERTICES,
                                               rec_sides_cur.data(),
                                               4,
                                               &e );
                if( e )
                    throw std::runtime_error( "Failed to extract target information\nfrom the captured frames!" );
                rect_sides_arr.emplace_back( rec_sides_cur );
            }

            ff = {}; // Release the frame, don't need it for progress callback

            if( progress_callback )
                progress_callback->on_update_progress( static_cast< float >( ++progress ) );
        }

        if( rect_sides_arr.size() )
        {
            for( int i = 0; i < 4; ++i )
                rect_sides[i] = rect_sides_arr[0][i];

            for( int j = 1; j < rect_sides_arr.size(); ++j )
            {
                for( int i = 0; i < 4; ++i )
                    rect_sides[i] += rect_sides_arr[j][i];
            }

            for( int i = 0; i < 4; ++i )
                rect_sides[i] /= rect_sides_arr.size();
        }
        else
            throw std::runtime_error( "Failed to extract the target rectangle info!" );
    }

    float ds_calib_common::get_focal_length_correction_factor( const float left_rect_sides[4],
                                                               const float right_rect_sides[4],
                                                               const float fx[2],
                                                               const float fy[2],
                                                               const float target_w,
                                                               const float target_h,
                                                               const float baseline,
                                                               float & ratio,
                                                               float & angle )
    {
        const float correction_factor = 0.5f;

        float ar[2] = { 0 };
        float tmp = left_rect_sides[2] + left_rect_sides[3];
        if( tmp > 0.1f )
            ar[0] = ( left_rect_sides[0] + left_rect_sides[1] ) / tmp;

        tmp = right_rect_sides[2] + right_rect_sides[3];
        if( tmp > 0.1f )
            ar[1] = ( right_rect_sides[0] + right_rect_sides[1] ) / tmp;

        float align = 0.0f;
        if( ar[0] > 0.0f )
            align = ar[1] / ar[0] - 1.0f;

        float ta[2] = { 0 };
        float gt[4] = { 0 };
        float ave_gt = 0.0f;

        if( left_rect_sides[0] > 0 )
            gt[0] = fx[0] * target_w / left_rect_sides[0];

        if( left_rect_sides[1] > 0 )
            gt[1] = fx[0] * target_w / left_rect_sides[1];

        if( left_rect_sides[2] > 0 )
            gt[2] = fy[0] * target_h / left_rect_sides[2];

        if( left_rect_sides[3] > 0 )
            gt[3] = fy[0] * target_h / left_rect_sides[3];

        ave_gt = 0.0f;
        for( int i = 0; i < 4; ++i )
            ave_gt += gt[i];
        ave_gt /= 4.0;

        ta[0] = atanf( align * ave_gt / std::abs( baseline ) );
        ta[0] = rad2deg( ta[0] );

        if( right_rect_sides[0] > 0 )
            gt[0] = fx[1] * target_w / right_rect_sides[0];

        if( right_rect_sides[1] > 0 )
            gt[1] = fx[1] * target_w / right_rect_sides[1];

        if( right_rect_sides[2] > 0 )
            gt[2] = fy[1] * target_h / right_rect_sides[2];

        if( right_rect_sides[3] > 0 )
            gt[3] = fy[1] * target_h / right_rect_sides[3];

        ave_gt = 0.0f;
        for( int i = 0; i < 4; ++i )
            ave_gt += gt[i];
        ave_gt /= 4.0;

        ta[1] = atanf( align * ave_gt / std::abs( baseline ) );
        ta[1] = rad2deg( ta[1] );

        angle = ( ta[0] + ta[1] ) / 2;

        align *= 100;

        float r[4] = { 0 };
        float c = fx[0] / fx[1];

        if( left_rect_sides[0] > 0.1f )
            r[0] = c * right_rect_sides[0] / left_rect_sides[0];

        if( left_rect_sides[1] > 0.1f )
            r[1] = c * right_rect_sides[1] / left_rect_sides[1];

        c = fy[0] / fy[1];
        if( left_rect_sides[2] > 0.1f )
            r[2] = c * right_rect_sides[2] / left_rect_sides[2];

        if( left_rect_sides[3] > 0.1f )
            r[3] = c * right_rect_sides[3] / left_rect_sides[3];

        float ra = 0.0f;
        for( int i = 0; i < 4; ++i )
            ra += r[i];
        ra /= 4;

        ra -= 1.0f;
        ra *= 100;

        ratio = ra - correction_factor * align;
        float ratio_to_apply = ratio / 100.0f + 1.0f;

        return ratio_to_apply;
    }

    void ds_calib_common::handle_calibration_error( int status )
    {
        switch( status )
        {
        case ds_calib_common::STATUS_EDGE_TOO_CLOSE:
            throw std::runtime_error( "Calibration didn't converge! - edges too close\n"
                                      "Please retry in different lighting conditions" );
        case ds_calib_common::STATUS_FILL_FACTOR_TOO_LOW:
                throw std::runtime_error( "Not enough depth pixels! - low fill factor)\n"
                                          "Please retry in different lighting conditions" );
        case ds_calib_common::STATUS_NOT_CONVERGE:
                throw std::runtime_error( "Calibration failed to converge\n"
                                          "Please retry in different lighting conditions" );
        case ds_calib_common::STATUS_NO_DEPTH_AVERAGE:
            throw std::runtime_error( "Calibration didn't converge! - no depth average\n"
                                          "Please retry in different lighting conditions" );
        default:
            throw std::runtime_error( rsutils::string::from() <<
                                      "Calibration didn't converge! (RESULT=" << int( status ) << ")" );
        }
    }
}  // namespace librealsense
