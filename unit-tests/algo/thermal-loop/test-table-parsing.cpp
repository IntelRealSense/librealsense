// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/thermal-loop/*.cpp
#include "../algo-common.h"
#include "create-synthetic-l500-thermal-table.h"
#include <src/algo/thermal-loop/l500-thermal-loop.h>


using namespace librealsense::algo::thermal_loop::l500;


TEST_CASE("parse_thermal_table", "[thermal-loop]")
{
    auto original_table = create_synthetic_table();
    auto raw_data = original_table.build_raw_data();
    thermal_calibration_table table_from_raw( raw_data );
    REQUIRE( original_table == table_from_raw );
}

TEST_CASE( "invalid thermal table", "[thermal-loop]" )
{
    auto table = create_synthetic_table();
    table._header.valid = 0.f;

    auto raw_data = table.build_raw_data();
    REQUIRE_THROWS( thermal_calibration_table( raw_data ));
}

TEST_CASE( "data_size_too_small", "[thermal-loop]" )
{
    auto syntetic_table = create_synthetic_table();
    auto raw_data = syntetic_table.build_raw_data();
    raw_data.pop_back();
    REQUIRE_THROWS( thermal_calibration_table( raw_data ) );
}

TEST_CASE( "data_size_too_large", "[thermal-loop]" )
{
    auto syntetic_table = create_synthetic_table();
    auto raw_data = syntetic_table.build_raw_data();
    raw_data.push_back( 1 );
    REQUIRE_THROWS( thermal_calibration_table( raw_data ) );
}

TEST_CASE( "build_raw_data", "[thermal-loop]" )
{
    auto syntetic_table = create_synthetic_table(1);
    auto raw_data = syntetic_table.build_raw_data();
   
    std::vector< byte > raw;
    
    raw.insert( raw.end(),
                (byte *)&( syntetic_table._header.min_temp ),
                (byte *)&( syntetic_table._header.min_temp ) + 4 );

    raw.insert( raw.end(),
                (byte *)&( syntetic_table._header.max_temp ),
                (byte *)&( syntetic_table._header.max_temp ) + 4 );

    raw.insert( raw.end(),
                (byte *)&( syntetic_table._header.reference_temp ),
                (byte *)&( syntetic_table._header.reference_temp ) + 4 );

    raw.insert( raw.end(),
                 (byte *)&( syntetic_table._header.valid ),
                 (byte *)&( syntetic_table._header.valid ) + 4 );

    for (auto v : syntetic_table.bins)
    {
        raw.insert( raw.end(),
                    (byte *)&( v.scale ),
                    (byte *)&( v.scale ) + 4 );

        raw.insert( raw.end(), (byte *)&( v.sheer ), (byte *)&( v.sheer ) + 4 );
        raw.insert( raw.end(), (byte *)&( v.tx ), (byte *)&( v.tx ) + 4 );
        raw.insert( raw.end(), (byte *)&( v.ty ), (byte *)&( v.ty ) + 4 );
    }

    CHECK( raw_data == raw );
    REQUIRE_THROWS( thermal_calibration_table( raw_data, 2 ) );
}

TEST_CASE( "build_raw_data_no_data", "[thermal-loop]" )
{
    auto syntetic_table = create_synthetic_table( 0 );
    auto raw_data = syntetic_table.build_raw_data();

    thermal_calibration_table t( raw_data, 0 );

    CHECK( t.bins.size() == 0 );
}
