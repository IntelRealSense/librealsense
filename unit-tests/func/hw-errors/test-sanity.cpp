// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


#include "../func-common.h"
#include "hw-errors-common.h"

using namespace rs2;

TEST_CASE( "Error handling sanity", "[l500][live]" )
{
    // Require at least one device to be plugged in
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    validate_errors_handling( dev, l500_error_report );
}
