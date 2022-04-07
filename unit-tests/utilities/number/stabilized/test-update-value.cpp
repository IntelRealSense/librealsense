// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../../include/librealsense2/utilities/number/stabilized-value.h


#include "../../../test.h"
#include <librealsense2/utilities/number/stabilized-value.h>

using namespace utilities::number;

// Test group description:
//       * This tests group verifies stabilized_value class.
//
// Current test description:
//       * Verify if history is filled with a stable value and then filled with required percentage
//         of new val, new val is returned as stable value.
TEST_CASE( "update stable value - nominal", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 10 );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );

    CHECK( 60.0f == stab_value.get( 0.6f ) );
    CHECK_NOTHROW( stab_value.add( 35.0f ) );
    CHECK_NOTHROW( stab_value.add( 35.0f ) );
    CHECK_NOTHROW( stab_value.add( 35.0f ) );
    CHECK_NOTHROW( stab_value.add( 35.0f ) );
    CHECK_NOTHROW( stab_value.add( 35.0f ) );
    CHECK( 60.0f == stab_value.get( 0.6f ) );

    CHECK_NOTHROW( stab_value.add( 35.0f ) );
    CHECK( 35.0f == stab_value.get( 0.6f ) );
}



TEST_CASE("update stable value - last stable not in history", "[stabilized value]")
{
    stabilized_value< float > stab_value(10);
    stab_value.add(55.0f);
    CHECK(55.0f == stab_value.get(1.0f));

    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    CHECK(55.0f == stab_value.get(1.0f));
    CHECK(60.0 == stab_value.get(0.9f));
    stab_value.add(70.0f);
    CHECK(60.0f == stab_value.get(1.0f));
}

TEST_CASE("update stable value - last stable is in history", "[stabilized value]")
{
    stabilized_value< float > stab_value(10);
    stab_value.add(55.0f);
    stab_value.add(60.0f);
    stab_value.add(60.0f);
    CHECK(55.0f == stab_value.get(0.8f));
}