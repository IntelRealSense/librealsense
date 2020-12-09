// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../../common/utilities/number/stabilized-value.h


#include "../../../test.h"
#include <../common/utilities/number/stabilized-value.h>

using namespace utilities::number;

// Test group description:
//       * This tests group verifies stabilized_value class.
//
// Current test description:
//       * Verify that empty function works as expected
TEST_CASE( "empty test", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 5 );
    CHECK( stab_value.empty() );
    CHECK_NOTHROW( stab_value.add( 1.0f ) );
    CHECK_FALSE( stab_value.empty() );
    stab_value.clear();
    CHECK( stab_value.empty() );
}
