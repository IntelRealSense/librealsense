// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/thermal-loop/*.cpp
#include "../algo-common.h"
#include "./create-syntetic-data .h"
#include "algo/thermal-loop/l500-thermal-loop.h"

using namespace librealsense::algo::thermal_loop::l500;



TEST_CASE("get_scale", "[thermal-loop]")
{
    std::map < double, double> temp_to_expected_scale= { 
            { -1, 0.5 }, // under minimum temp
            { 0, 0.5},
            { 2.5, 0.5 },
            { 3.56756, 1 },
            { 5, 1 },
            { 7.5, 1.5 },
            { 7.6, 2 },
            { 73, 14.5 },
            { 75, 14.5 }, // maximum temp
            { 78, 14.5 }, // above maximum temp
    };
    

    auto syntetic_table = create_syntetic_table();
    // [0   - 2.5]  --> 0.5
    // (2.5 - 5]    --> 1
    // (5   - 7.5]  --> 1.5
    // (7.5 - 10]   --> 2
    // ...
    // (72.5 - 75]  --> 15

    for (auto temp_scale : temp_to_expected_scale)
    {
        REQUIRE( syntetic_table.get_current_thermal_scale( temp_scale.first )
                 == 1. / temp_scale.second );
    }
}

TEST_CASE( "correct_thermal_scale", "[thermal-loop]" )
{
    auto res = correct_thermal_scale( { 50, 100 }, 2.5 );
    REQUIRE( res.first == 125 );
    REQUIRE( res.second == 250 );

    res = correct_thermal_scale( { 50, 100 }, 3.555 );
    REQUIRE( res.first == 177.75 );
    REQUIRE( res.second == 355.5 );
}