// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

// This test is check that after setting gain high or low on automatic preset
// the current values don't change and the default vaules are update according to gain
TEST_CASE( "gain changed after preset auto ", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_L500);
    auto dev = devices[0];

    exit_if_fw_version_is_under(dev, MIN_GET_DEFAULT_FW_VERSION);

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto preset_mode_def = get_camera_preset_mode_defaults(depth_sens);

    std::map< rs2_option, float > expected_values, expected_defs;

    REQUIRE_NOTHROW(depth_sens.set_option(RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA));

    for (int gain = RS2_DIGITAL_GAIN_HIGH; gain <= RS2_DIGITAL_GAIN_LOW; gain++)
    {
        auto expected_gain_defs = build_gain_to_expected_defaults(dev, depth_sens, (rs2_digital_gain)gain, RS2_SENSOR_MODE_VGA);
        REQUIRE_NOTHROW(
            depth_sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_AUTOMATIC));

        build_preset_to_expected_values_map(dev, depth_sens, { RS2_L500_VISUAL_PRESET_AUTOMATIC,RS2_SENSOR_MODE_VGA }, expected_values, expected_defs);

        depth_sens.set_option(RS2_OPTION_DIGITAL_GAIN, gain);

        REQUIRE_NOTHROW(
            (rs2_l500_visual_preset)(int)depth_sens.get_option(RS2_OPTION_VISUAL_PRESET)
            == RS2_L500_VISUAL_PRESET_CUSTOM);

        expected_values[RS2_OPTION_DIGITAL_GAIN] = gain;

        compare_to_actual(depth_sens, expected_values, expected_gain_defs);
    }
}