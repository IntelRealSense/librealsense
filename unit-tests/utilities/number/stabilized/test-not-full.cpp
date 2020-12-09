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
//       * Verify if history is not the logic works as expected and the percentage is calculated
//       from the history current size
TEST_CASE( "not full history", "[stabilized value]" )
{
    stabilized_value< float > stab_value( 30 );
    CHECK_NOTHROW( stab_value.add( 76.0f ) );
    CHECK_NOTHROW( stab_value.add( 76.0f ) );
    CHECK_NOTHROW( stab_value.add( 76.0f ) );
    CHECK_NOTHROW( stab_value.add( 76.0f ) );
    CHECK( 76.0f == stab_value.get( 0.6f ) );

    CHECK_NOTHROW( stab_value.add( 45.0f ) );
    CHECK_NOTHROW( stab_value.add( 45.0f ) );
    CHECK_NOTHROW( stab_value.add( 45.0f ) );
    CHECK_NOTHROW( stab_value.add( 45.0f ) );
    CHECK_NOTHROW( stab_value.add( 45.0f ) );

    CHECK( 76.0f == stab_value.get( 0.6f ) );
    CHECK_NOTHROW( stab_value.add( 45.0f ) );  // The stable value should change now (4 * 76.0 +
                                               // 6 * 45.0 (total 10 values))
    CHECK( 45.0f == stab_value.get( 0.6f ) );
}
