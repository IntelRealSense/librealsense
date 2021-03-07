// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L515

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

// This test is check that after setting gain to auto on automatic preset
// the visual preset is calculted correctlly
TEST_CASE( "check currents and defaults values after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_L500);
    auto dev = devices[0];

    exit_if_fw_version_is_under(dev, MIN_GET_DEFAULT_FW_VERSION);

    auto depth_sens = dev.first< rs2::depth_sensor >();

    reset_camera_preset_mode_to_defaults(depth_sens);

    depth_sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_AUTOMATIC);
    depth_sens.set_option(RS2_OPTION_ALTERNATE_IR, 0);

    depth_sens.set_option(RS2_OPTION_DIGITAL_GAIN, RS2_DIGITAL_GAIN_AUTO);

    REQUIRE_NOTHROW(
        (rs2_l500_visual_preset)(int)depth_sens.get_option(RS2_OPTION_VISUAL_PRESET)
        == RS2_L500_VISUAL_PRESET_CUSTOM);


    depth_sens.set_option(RS2_OPTION_ALTERNATE_IR, 1);

    REQUIRE_NOTHROW(
        (rs2_l500_visual_preset)(int)depth_sens.get_option(RS2_OPTION_VISUAL_PRESET)
        == RS2_L500_VISUAL_PRESET_AUTOMATIC);
}
