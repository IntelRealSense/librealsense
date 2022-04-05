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
//       * Verify if history is filled with a stable value at the required percentage , the user
//         will get it when asked for even if other inputs exist in history

TEST_CASE( "get 60% stability", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 10 );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 55.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );
    CHECK_NOTHROW( stab_value.add( 60.0f ) );

    CHECK( 55.0f == stab_value.get( 0.6f ) );
}
