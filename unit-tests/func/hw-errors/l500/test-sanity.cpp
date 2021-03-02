// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.


#include "../../func-common.h"
#include "l500-error-common.h"
#include "../error-common.h"

//#test:device L500*

using namespace rs2;

TEST_CASE( "Error handling sanity", "[l500][live]" )
{
    // Require at least one device to be plugged in
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    validate_errors_handling( dev, build_log_errors_map());
}

TEST_CASE("Error handling while streaming", "[l500][live]")
{
    // Require at least one device to be plugged in
    auto devices = find_devices_by_product_line_or_exit(RS2_PRODUCT_LINE_L500);
    auto dev = devices[0];
    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto depth = find_default_depth_profile(depth_sens);
    auto ir = find_default_ir_profile(depth_sens);
    auto confidence = find_confidence_corresponding_to_depth(depth_sens, depth);

    do_while_streaming(depth_sens, { depth, ir, confidence }, [&]() {
        validate_errors_handling(dev, build_log_errors_map());
    });
}
