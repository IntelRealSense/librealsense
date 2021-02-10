// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

bool is_controls_values_changed_after_move_to_auto(rs2::sensor sens)
{
    std::map<rs2_option, float> option_to_vals;

    for (auto o : auto_dependent_options)
    {
        REQUIRE_NOTHROW(option_to_vals[o] = sens.get_option_range(o).def);
    }

    REQUIRE_NOTHROW(
        sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_AUTOMATIC));

    for (auto o : option_to_vals)
    {
        float val;
        REQUIRE_NOTHROW(val = sens.get_option_range(o.first).def);
        if (val != o.second)
            return true;
    }
    return false;
}

// This test checks that we reset auto dependent options (apd, laser and min distance)
// when move to auto
TEST_CASE( "move from regular preset to automatic ", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto sens = dev.first< rs2::depth_sensor >();

    REQUIRE_NOTHROW(
        sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_NO_AMBIENT));

    // if auto dependent options didnt changed after moving to auto from high gain
    // they must changed after moving to auto from low gain
    // this is show that we reset the dependent options correctlly
    if(!is_controls_values_changed_after_move_to_auto(sens))
    {
        REQUIRE_NOTHROW(
            sens.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_SHORT_RANGE));
       
        // check the auto dependent options changed after moving to auto from low gain
        REQUIRE(is_controls_values_changed_after_move_to_auto(sens));
    }

}
