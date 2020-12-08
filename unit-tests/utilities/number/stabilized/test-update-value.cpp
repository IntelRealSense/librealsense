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
//       * Verify if history is filled with a stable value and then filled with required percentage
//         of new val, new val is returned as stable value.
TEST_CASE( "update stable value", "[stabilized value]" )
{
    try
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
    catch( const std::exception & e )
    {
        FAIL( "Exception caught: " << e.what() );
    }
}