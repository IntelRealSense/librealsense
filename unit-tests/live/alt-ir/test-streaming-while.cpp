// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#test:device L500*

#include "../live-common.h"
#include "alt-ir-common.h"

using namespace rs2;

TEST_CASE( "Enable AltIR while streaming and check that all streams arrived", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();
    if( alt_ir_supported_or_message( depth_sens ) )
    {
        auto depth = find_default_depth_profile( depth_sens );
        auto ir = find_default_ir_profile( depth_sens );
        auto confidence = find_confidence_corresponding_to_depth( depth_sens, depth );

        enable_alt_ir_and_check_that_all_streams_arrived( dev,
                                                          depth_sens,
                                                          { depth, ir, confidence } );
    }
}
