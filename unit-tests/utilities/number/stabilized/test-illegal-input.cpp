// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../../common/utilities/number/stabilized-value.h


#include "../../../test.h"
#include "../common/utilities/number/stabilized-value.h"

using namespace utilities::number;

// Test group description:
//       * This tests group verifies stabilized_value class.
//
// Current test description:
//       * Verify stabilized_value percentage input is at range (0-100] % (zero not included)
TEST_CASE( "Illegal input - percentage value too high", "[stabilized value]" )
{
    try
    {
        stabilized_value< float > stab_value1( 5, 1.1f );
        FAIL( "percentage over 100% must throw" );
    }
    catch( ... )
    {
        SUCCEED();
    }
}

TEST_CASE( "Illegal input - percentage value too low", "[stabilized value]" )
{
    try
    {
        stabilized_value< float > stab_value3( 5, -1.1f );
        FAIL( "negative percentage must throw" );
    }
    catch( ... )
    {
        SUCCEED();
    }
}

TEST_CASE( "Illegal input - percentage value is zero", "[stabilized value]" )
{
    try
    {
        stabilized_value< float > stab_value( 5, 0.0f );
        FAIL( "zero percentage must throw" );
    }
    catch( ... )
    {
        SUCCEED();
    }
}