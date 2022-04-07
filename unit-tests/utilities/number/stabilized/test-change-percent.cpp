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
//       * Verify that on the same values different 'get' calls with different stabilization percent
//         give the expected value
TEST_CASE( "change stabilization percent", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 5 );
    CHECK_NOTHROW( stab_value.add( 1.0f ) );
    CHECK_NOTHROW( stab_value.add( 1.0f ) );
    CHECK_NOTHROW( stab_value.add( 1.0f ) );
    CHECK_NOTHROW( stab_value.add( 1.0f ) );
    CHECK_NOTHROW( stab_value.add( 1.0f ) );

    CHECK_NOTHROW( stab_value.add( 2.0f ) );
    CHECK_NOTHROW( stab_value.add( 2.0f ) );
    CHECK_NOTHROW( stab_value.add( 2.0f ) );
    CHECK_NOTHROW( stab_value.add( 2.0f ) );

    CHECK( 1.0f == stab_value.get( 1.0f ) );
    CHECK( 1.0f == stab_value.get( 0.9f ) );
    CHECK( 2.0f == stab_value.get( 0.8f ) );
}
