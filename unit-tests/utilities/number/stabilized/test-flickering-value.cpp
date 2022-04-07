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
//       * Verify a stabilized value once the inserted value is flickering between 2 values
TEST_CASE( "update stable value", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 10 );

    // Verify flickering value always report the stable value
    for( int i = 0; i < 100; i++ )
    {
        stab_value.add( 55.0f );
        stab_value.add( 60.0f );
        CHECK( 55.0f == stab_value.get( 0.7f ) );
    }
}