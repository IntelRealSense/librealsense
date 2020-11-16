// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Unit Test Goals:
// Test the get_manufacture_work_week function

//#cmake:add-file ../../../common/utilities/time/work_week.h
//#cmake:add-file ../../../common/utilities/time/work_week.cpp


#include "common.h"
#include "../common/utilities/time/work_week.h"

#define INVALID_MESSAGE( serial, invalid ) "Invalid serial number \"" + serial + "\" " + invalid

using namespace utilities::time;

// Test description:
// > Test error throws fron the get_manufacture_work_week function
TEST_CASE( "test get_manufacture_work_week", "[work_week]" )
{
    std::string serial = "xxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ),
                       INVALID_MESSAGE( serial, "length" ) );

    serial = "xxxxxxxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ), 
                       INVALID_MESSAGE( serial, "year" ) );

    serial = "x9xxxxxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ),
                       INVALID_MESSAGE( serial, "work week" ) );

    serial = "x92xxxxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ),
                       INVALID_MESSAGE( serial, "work week" ) );

    serial = "x9x5xxxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ),
                       INVALID_MESSAGE( serial, "work week" ) );

    serial = "x962xxxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ),
                       INVALID_MESSAGE( serial, "work week" ) );

    serial = "x955xxxx";
    CHECK_THROWS_WITH( l500::get_manufacture_work_week( serial ),
                       INVALID_MESSAGE( serial, "work week" ) );

    serial = "x925xxxx";
    CHECK_NOTHROW( l500::get_manufacture_work_week( serial ) );
}