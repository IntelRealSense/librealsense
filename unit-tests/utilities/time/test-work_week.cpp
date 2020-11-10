// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the work_week class

//#cmake:add-file ../../../common/utilities/time/work_week.h
//#cmake:add-file ../../../common/utilities/time/work_week.cpp


#include "common.h"
#include "../common/utilities/time/work_week.h"

using namespace utilities::time;

// Test description:
// > Test the work_week main functions
TEST_CASE( "test get_work_weeks_since", "[work_week]" )
{
    REQUIRE( get_work_weeks_since( work_week::current() ) == 0 );

    work_week zero( 0, 0 );
    REQUIRE( get_work_weeks_since( zero ) == work_week::current() - zero );
}