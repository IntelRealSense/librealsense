// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

#include "common.h"
#include <rsutils/time/work-week.h>
#include <ctime>

using namespace rsutils::time;

// Test description:
// > Test the c'tor
TEST_CASE( "test work_week c'tor", "[work_week]" )
{
    tm set_time = { 0 };
    set_time.tm_year = 117;  // 2017
    set_time.tm_mday = 1;
    set_time.tm_isdst = -1;
    // The work week of the first day of the year is 1 because work weeks are 1 based
    work_week first_day( std::mktime( &set_time ) );
    CHECK( first_day.get_work_week() == 1 );
    set_time.tm_mday = 7;
    set_time.tm_wday = 6;
    // This should be the last day of the first work week because 2017 started on Sunday
    work_week sixth_day( std::mktime( &set_time ) );
    CHECK( sixth_day.get_work_week() == 1 );
    set_time.tm_mday = 8;
    set_time.tm_wday = 0;
    // This should be the first day of the second work week
    work_week seventh_day( std::mktime( &set_time ) );
    CHECK( seventh_day.get_work_week() == 2 );
    // If the year didn't start on a Sunday (for example 2018), Jan 7th should be in work week 2
    set_time.tm_year = 118;  // 2018
    set_time.tm_mday = 7;
    work_week second_week( std::mktime( &set_time ) );
    CHECK( second_week.get_work_week() == 2 );

    // Checking valid and invalid wprk weeks
    CHECK_THROWS_WITH( work_week( 2020, 0 ),
                       "Invalid work week given: 2020 doesn't have a work week 0" );
    CHECK_THROWS_WITH( work_week( 2020, 53 ),
                       "Invalid work week given: 2020 doesn't have a work week 53" );
    CHECK_NOTHROW( work_week( 2016, 53 ) );

    // checking edge case of 31st of December is actualy part of work week 1 of the next year
    set_time.tm_year = 117;  // 2017
    set_time.tm_mon = 11;    // December (it's 0 based)
    set_time.tm_mday = 31;
    work_week belons_to_next_year( std::mktime( &set_time ) );
    CHECK( belons_to_next_year.get_work_week() == 1 );
    CHECK( belons_to_next_year.get_year() == 2018 );
}

// Test description:
// > Test all work_week creation options are equivalent
TEST_CASE( "test work_week c'tor equivalence", "[work_week]" )
{
    // Compare current with c'tor using current time
    auto current_ww = work_week::current();
    auto now = std::time( nullptr );
    work_week now_to_work_week( now );
    CHECK( current_ww.get_year() == now_to_work_week.get_year() );
    CHECK( current_ww.get_work_week() == now_to_work_week.get_work_week() );

    // Test copy c'tor
    work_week copy( current_ww );
    CHECK( current_ww.get_year() == copy.get_year() );
    CHECK( current_ww.get_work_week() == copy.get_work_week() );

    // Compare manual c'tor with c'tor from given time
    tm set_time = { 0 };
    set_time.tm_year = 117;  // 2017
    set_time.tm_mday = 1;
    set_time.tm_isdst = -1;
    work_week manual( 2017, 1 );
    work_week from_time( std::mktime( &set_time ) );
    CHECK( manual.get_year() == from_time.get_year() );
    CHECK( manual.get_work_week() == from_time.get_work_week() );
}

// Test description:
// > Test the subtraction operator for work_week
TEST_CASE( "test work_week subtraction", "[work_week]" )
{
    // Simple cases
    CHECK( work_week( 2019, 7 ) - work_week( 2018, 7 ) == 52 );
    CHECK( work_week( 2019, 7 ) - work_week( 2018, 3 ) == 56 );
    CHECK( work_week( 2019, 7 ) - work_week( 2018, 10 ) == 49 );

    // Simple cases with negative results
    CHECK( work_week( 2018, 7 ) - work_week( 2019, 7 ) == -52 );
    CHECK( work_week( 2018, 3 ) - work_week( 2019, 7 ) == -56 );
    CHECK( work_week( 2018, 10 ) - work_week( 2019, 7 ) == -49 );

    // 2016 had 53 work weeks
    CHECK( work_week( 2017, 7 ) - work_week( 2016, 7 ) == 53 );
    CHECK( work_week( 2017, 7 ) - work_week( 2016, 3 ) == 57 );
    CHECK( work_week( 2017, 7 ) - work_week( 2016, 10 ) == 50 );
    CHECK( work_week( 2017, 1 ) - work_week( 2016, 52 ) == 2 );

    // Subtraction of consecutive weeks
    CHECK( work_week( 2017, 1 ) - work_week( 2016, 53 ) == 1 );
    tm Jan_1_2017 = { 0 };
    Jan_1_2017.tm_year = 117;  // 2017
    Jan_1_2017.tm_mon = 0;     // January (it's 0 based)
    Jan_1_2017.tm_mday = 1;
    Jan_1_2017.tm_wday = 0;
    Jan_1_2017.tm_yday = 0;
    Jan_1_2017.tm_isdst = -1;
    tm Dec_31_2016 = { 0 };
    Dec_31_2016.tm_year = 116;  // 2016
    Dec_31_2016.tm_mon = 11;    // December (it's 0 based)
    Dec_31_2016.tm_mday = 31;
    Dec_31_2016.tm_wday = 5;
    Dec_31_2016.tm_yday = 365;
    Dec_31_2016.tm_isdst = -1;
    CHECK( work_week( std::mktime( &Jan_1_2017 ) ) - work_week( std::mktime( &Dec_31_2016 ) )
           == 1 );

    // Subtracting days from the same work week. December 31st 2017 is part of work week 1 of 2018
    tm Jan_1_2018 = { 0 };
    Jan_1_2018.tm_year = 118;  // 2018
    Jan_1_2018.tm_mon = 0;     // January (it's 0 based)
    Jan_1_2018.tm_mday = 1;
    Jan_1_2018.tm_isdst = -1;
    tm Dec_31_2017 = { 0 };
    Dec_31_2017.tm_year = 117;  // 2017
    Dec_31_2017.tm_mon = 11;    // December (it's 0 based)
    Dec_31_2017.tm_mday = 31;
    Dec_31_2017.tm_isdst = -1;
    CHECK( work_week( std::mktime( &Jan_1_2018 ) ) - work_week( std::mktime( &Dec_31_2017 ) )
           == 0 );
}

// Test description:
// > Test the get_work_weeks_since function
TEST_CASE( "test get_work_weeks_since", "[work_week]" )
{
    CHECK( get_work_weeks_since( work_week::current() ) == 0 );

    work_week zero( 0, 1 );
    CHECK( get_work_weeks_since( zero ) == work_week::current() - zero );
}
