// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


/*
This class is for any use that requires work weeks.
Read more on work weeks at https://en.wikipedia.org/wiki/ISO_week_date
Note that we use the US accounting method, in which the 1st of January is always in work week 1.
for example, 1/1/2022 is a Saturday, so that whole week is work week 1, i.e 26-31/12/2021 are
actualy in work week 1 of 2022
*/
#pragma once

#include <string>
#include <ctime>


namespace rsutils {
namespace time {

class work_week
{
    unsigned _year;
    unsigned _ww; //starts at 1

public:
    work_week(unsigned year, unsigned ww);
    work_week( const std::time_t & time );
    static work_week current();
    work_week(const work_week & ) = default;

    unsigned get_year() const;
    unsigned get_work_week() const;
    int operator-( const work_week & ww ) const;
};

// Returns the number of work weeks since given time
unsigned get_work_weeks_since( const work_week & start );

// Calulates and returns the Julian day number of the given date. 
//read more in https://en.wikipedia.org/wiki/Julian_day
unsigned jdn( unsigned year, unsigned month, unsigned day );
}  // namespace time
}  // namespace rsutils
