// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "automatic preset sanity ", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto sens = dev.first< rs2::depth_sensor >();

    reset_camera_preset_mode_to_defaults(sens);

    REQUIRE_NOTHROW(
        sens.set_option(RS2_OPTION_ALTERNATE_IR, 0));

    REQUIRE_NOTHROW(
        sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_AUTOMATIC));

    REQUIRE(sens.get_option(RS2_OPTION_VISUAL_PRESET) == RS2_L500_VISUAL_PRESET_AUTOMATIC);
    REQUIRE(sens.get_option(RS2_OPTION_DIGITAL_GAIN) == RS2_DIGITAL_GAIN_AUTO);
    REQUIRE(sens.is_option_read_only(RS2_OPTION_AVALANCHE_PHOTO_DIODE));
    REQUIRE(sens.get_option_range(RS2_OPTION_AVALANCHE_PHOTO_DIODE).def == -1);
    REQUIRE(sens.get_option(RS2_OPTION_ALTERNATE_IR) == 1);

    REQUIRE_NOTHROW(
        sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_NO_AMBIENT));

    REQUIRE(sens.get_option(RS2_OPTION_ALTERNATE_IR) == 0);
}
