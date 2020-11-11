// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


#include "../../tests.h"
#include "../utilities.h"

using namespace rs2;

TEST_CASE( "AC fails if AltIR was enable after stream start", "[l500][live]" )
{
    auto devices = find_device_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    std::string serial;
    REQUIRE_NOTHROW( serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER ) );


    auto depth_sens = get_depth_sensor( dev );

    const uint32_t MIN_FW_VER[4] = { 1, 5, 2, 0 };
    if( is_fw_version_newer( depth_sens, MIN_FW_VER ) )
    {
        auto profiles = find_profiles_to_stream( depth_sens );
        enable_alt_ir_and_check_that_AC_fails( dev, get_depth_sensor( dev ),
                                               profiles );
    }
    else
        std::cout << depth_sens.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION )
                  << " doesn't support alt IR option";

}