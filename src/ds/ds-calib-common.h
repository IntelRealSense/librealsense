// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/hpp/rs_types.hpp>

#include <map>

namespace librealsense
{
    class auto_calibrated_interface;
    class sensor_interface;

    // RAII to handle auto-calibration with the thermal compensation disabled
    class thermal_compensation_guard
    {
    public:
        thermal_compensation_guard( auto_calibrated_interface * handle );
        virtual ~thermal_compensation_guard();

    protected:
        bool restart_tl;
        librealsense::sensor_interface* snr;

    private:
        // Disable copy/assignment ctors
        thermal_compensation_guard( const thermal_compensation_guard & ) = delete;
        thermal_compensation_guard & operator=( const thermal_compensation_guard & ) = delete;
    };

    class ds_calib_common
    {
    public:
        enum auto_calib_speed : uint8_t
        {
            SPEED_VERY_FAST  = 0,
            SPEED_FAST       = 1,
            SPEED_MEDIUM     = 2,
            SPEED_SLOW       = 3,
            SPEED_WHITE_WALL = 4
        };

        enum dsc_status : uint16_t
        {
            STATUS_SUCCESS             = 0,
            STATUS_RESULT_NOT_READY    = 1, // Self calibration result is not ready yet
            STATUS_FILL_FACTOR_TOO_LOW = 2, // There are too little textures in the scene
            STATUS_EDGE_TOO_CLOSE      = 3, // Self calibration range is too small
            STATUS_NOT_CONVERGE        = 4, // For tare calibration only
            STATUS_BURN_SUCCESS        = 5,
            STATUS_BURN_ERROR          = 6,
            STATUS_NO_DEPTH_AVERAGE    = 7
        };

#pragma pack( push, 1 )
        struct dsc_check_status_result
        {
            uint16_t status;
            uint16_t stepCount;
            uint16_t stepSize;            // 1/1000 of a pixel
            uint32_t pixelCountThreshold; // minimum number of pixels in selected bin
            uint16_t minDepth;            // Depth range for FWHM
            uint16_t maxDepth;
            uint32_t rightPy;             // 1/1000000 of normalized unit
            float healthCheck;
            float rightRotation[9];
        };

        struct dsc_result
        {
            uint16_t paramSize;
            uint16_t status;
            float healthCheck;
            uint16_t tableSize;
        };
#pragma pack( pop )

        enum data_sampling
        {
            POLLING   = 0,
            INTERRUPT = 1
        };

        enum scan_parameter
        {
            PY_SCAN = 0,
            RX_SCAN = 1
        };

        enum auto_calib_sub_cmd : uint8_t
        {
            PY_RX_CALIB_CHECK_STATUS       = 0X03,
            INTERACTIVE_SCAN_CONTROL       = 0X05,
            PY_RX_CALIB_BEGIN              = 0X08,
            TARE_CALIB_BEGIN               = 0X0B,
            TARE_CALIB_CHECK_STATUS        = 0X0C,
            GET_CALIBRATION_RESULT         = 0X0D,
            FOCAL_LENGTH_CALIB_BEGIN       = 0X11,
            GET_FOCAL_LEGTH_CALIB_RESULT   = 0X12,
            PY_RX_PLUS_FL_CALIB_BEGIN      = 0X13,
            GET_PY_RX_PLUS_FL_CALIB_RESULT = 0X14,
            SET_COEFFICIENTS               = 0X19
        };

        // Convert json to string-int pairs.
        static std::map< std::string, int > parse_json( const std::string & json_content );

        // If map contains requested key-value pair, update value.
        static void update_value_if_exists( const std::map< std::string, int > & jsn, const std::string & key, int & value );

        // Check validity of common parameters.
        static void check_params( int speed, int scan_parameter, int data_sampling );

        // Checks validity of focal length caliration parameters. Throws on error.
        static void check_focal_length_params( int step_count,
                                               int fy_scan_range,
                                               int keep_new_value_after_sucessful_scan,
                                               int interrrupt_data_samling,
                                               int adjust_both_sides,
                                               int fl_scan_location,
                                               int fy_scan_direction,
                                               int white_wall_mode );

        // Calculate rectangle sides as shown in the frames. Also outputs current profile focal lenght.
        static void get_target_rect_info( rs2_frame_queue * frames,
                                          float rect_sides[4],
                                          float & fx,
                                          float & fy,
                                          int progress,
                                          rs2_update_progress_callback_sptr progress_callback );

        // Get scale factor to adjust focal lenght by. Also outputs ratio as percents and the detected tilt angle.
        static float get_focal_length_correction_factor( const float left_rect_sides[4],
                                                         const float right_rect_sides[4],
                                                         const float fx[2],
                                                         const float fy[2],
                                                         const float target_w,
                                                         const float target_h,
                                                         const float baseline,
                                                         float & ratio,
                                                         float & angle );

        static void handle_calibration_error( int status );
    };
}
