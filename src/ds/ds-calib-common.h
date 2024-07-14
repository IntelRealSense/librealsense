// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/hpp/rs_types.hpp>

namespace librealsense
{
    class ds_calib_common
    {
    public:
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
        static float get_focal_length_correction_factor( float left_rect_sides[4],
                                                         float right_rect_sides[4],
                                                         float fx[2], float fy[2],
                                                         float target_w, float target_h,
                                                         float baseline,
                                                         float & ratio,
                                                         float & angle );
    };
}
