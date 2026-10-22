// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include "viewer-test-helpers.h"


// Walk the object-detection sensor's Detection Distance combo through every mode and check that the
// overlay mode follows and the camera's control holds what the mode needs (on only for On-Camera)
VIEWER_TEST( "controls", "detection_distance_mode" )
{
    auto & model = test.find_first_device_or_exit();

    struct step
    {
        const char * label;
        rs2::detection_distance_mode mode;
        float fw_value;
    };
    step const steps[] = { { "Off", rs2::detection_distance_mode::off, 0.f },
                           { "Host", rs2::detection_distance_mode::host, 0.f },
                           { "On-Camera", rs2::detection_distance_mode::on_camera, 1.f },
                           { "Host", rs2::detection_distance_mode::host, 0.f } };

    for( auto && sub : model.subdevices )
    {
        if( ! test.has_option( sub, RS2_OPTION_DETECTION_DISTANCE ) || ! sub->detected_objects )
            continue;   // only object-detection sensors with the control (D585 FW 7.58.40802+)

        // Start from the camera's own state
        float const initial = sub->s->get_option( RS2_OPTION_DETECTION_DISTANCE );
        IM_CHECK( sub->detected_objects->distance_mode
                  == ( initial > 0.f ? rs2::detection_distance_mode::on_camera : rs2::detection_distance_mode::off ) );

        test.expand_sensor_panel( model, sub );
        for( auto const & s : steps )
        {
            test.select_control_combo_item( model, sub, RS2_OPTION_DETECTION_DISTANCE, s.label );
            IM_CHECK( sub->detected_objects->distance_mode == s.mode );
            IM_CHECK( test.wait_until( 10, 0.5f, [&] {
                return sub->s->get_option( RS2_OPTION_DETECTION_DISTANCE ) == s.fw_value;
            } ) );
        }

        // Leave the camera as we found it
        test.select_control_combo_item( model, sub, RS2_OPTION_DETECTION_DISTANCE, initial > 0.f ? "On-Camera" : "Off" );
        test.collapse_sensor_panel( model, sub );
    }
}
