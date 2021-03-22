// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.


#include "../../func-common.h"
#include "l500-error-common.h"
#include "../error-common.h"

//#test:device L500*

using namespace rs2;

// Return the same device (the first device found) to avoid unnecessary reinitializations
static rs2::device get_device()
{
    static rs2::device dev = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 )[0];
    return dev;
}

TEST_CASE( "Create device", "[l500][live]" )
{
    get_device();
}

TEST_CASE( "Non-streaming", "[l500][live]" )
{
    validate_errors_handling( get_device(), build_log_errors_map() );
}

TEST_CASE( "Streaming", "[l500][live]")
{
    // Require at least one device to be plugged in
    auto dev = get_device();
    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto depth = find_default_depth_profile( depth_sens );
    auto ir = find_default_ir_profile( depth_sens );
    auto confidence = find_confidence_corresponding_to_depth( depth_sens, depth );

    do_while_streaming( depth_sens, { depth, ir, confidence }, [&]() {
        validate_errors_handling( dev, build_log_errors_map() );
    } );
}
