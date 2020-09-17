// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/thermal-loop/*.cpp
#include "../algo-common.h"
#include "./create-synthetic-l500-thermal-table.h"
#include "algo/thermal-loop/l500-thermal-loop.h"


using namespace librealsense::algo::thermal_loop::l500;


TEST_CASE("parse_thermal_table", "[thermal-loop]")
{
    auto syntetic_table = create_synthetic_table();
    auto raw_data = syntetic_table.build_raw_data();
    auto parsed_table = thermal_calibration_table::parse_thermal_table( raw_data );
    REQUIRE(syntetic_table == parsed_table);
}

TEST_CASE( "data_size_too_small", "[thermal-loop]" )
{
    auto syntetic_table
        = create_synthetic_table( thermal_calibration_table::resolution - 1 );  // size too small 
    auto raw_data = syntetic_table.build_raw_data();
    REQUIRE_THROWS( thermal_calibration_table::parse_thermal_table( raw_data ));
}

TEST_CASE( "data_size_too_large", "[thermal-loop]" )
{
    auto syntetic_table
        = create_synthetic_table( thermal_calibration_table::resolution + 1 );  // size too small
    auto raw_data = syntetic_table.build_raw_data();
    REQUIRE_THROWS( thermal_calibration_table::parse_thermal_table( raw_data ) );
}