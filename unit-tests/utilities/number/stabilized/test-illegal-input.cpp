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
//       * Verify stabilized_value percentage input is at range (0-100] % (zero not included)
TEST_CASE( "Illegal input - percentage value too high", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 5 );
    stab_value.add( 55.f );
    CHECK_THROWS( stab_value.get( 1.1f ) );
    CHECK_THROWS( stab_value.get( -1.1f ) );
    CHECK_THROWS( stab_value.get( 0.0f ) );
}
