// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../common/utilities/time/work_week.h
//#cmake:add-file ../../../common/utilities/time/work_week.cpp


#include "common.h"
#include "../common/utilities/time/work_week.h"
#include <ctime>

using namespace utilities::time;

// Test description:
// > Test the c'tor
TEST_CASE("test work_week c'tor", "[work_week]")
{
    auto now = std::time(nullptr);
    auto tm = std::localtime(&now);
    tm->tm_yday = 0; // This feild hold the number of days since the 1st of January and is 0 based
    // The work week of the first day of the year is 1 because work weeks are 1 based
    work_week first_day(std::mktime(tm));
    CHECK(first_day.get_work_week() == 1);
    tm->tm_yday = 6;
    // This should be the last day of the first work week
    work_week sixth_day(std::mktime(tm));
    CHECK(sixth_day.get_work_week() == 1);
    tm->tm_yday = 7;
    // This should be the first day of the second work week
    work_week seventh_day(std::mktime(tm));
    CHECK(seventh_day.get_work_week() == 2);
}

// Test description:
// > Test all work_week creation options are equivalent
TEST_CASE("test work_week c'tor equivalence", "[work_week]")
{
    // Compare current with c'tor using current time
    auto current_ww = work_week::current();
    auto now = std::time(nullptr);
    work_week now_to_work_week(now);
    CHECK(current_ww.get_year() == now_to_work_week.get_year());
    CHECK(current_ww.get_work_week() == now_to_work_week.get_year());

    // Test copy c'tor
    work_week copy(current_ww);
    CHECK(current_ww.get_year() == copy.get_year());
    CHECK(current_ww.get_work_week() == copy.get_year());

    // Compare manual c'tor with c'tor from given time
    auto tm = std::localtime(&now);
    tm->tm_year = 0;
    tm->tm_yday = 5;
    work_week manual(1900, 0);
    work_week from_time(std::mktime(tm));
    CHECK(manual.get_year() == from_time.get_year());
    CHECK(manual.get_work_week() == from_time.get_work_week());
}

// Test description:
// > Test the subtraction operator for work_week
TEST_CASE("test work_week subtraction", "[work_week]")
{
    CHECK(work_week(3, 5) - work_week(1, 2) == 107);

    CHECK(work_week(3, 5) - work_week(1, 7) == 102);

    CHECK(work_week(1, 2) - work_week(3, 5) == -107);

    CHECK(work_week(1, 7) - work_week(3, 5) == -102);
}

// Test description:
// > Test the get_work_weeks_since function
TEST_CASE( "test get_work_weeks_since", "[work_week]" )
{
    CHECK( get_work_weeks_since( work_week::current() ) == 0 );

    work_week zero( 0, 0 );
    CHECK( get_work_weeks_since( zero ) == work_week::current() - zero );
}