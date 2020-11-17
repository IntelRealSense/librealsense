// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <ctime>


namespace utilities {
namespace time {

class work_week
{
    unsigned _year;
    unsigned _ww; //starts at 1

public:
    work_week( unsigned year, unsigned ww )
        : _year( year )
        , _ww( ww )
    {
    }
    work_week( const std::time_t & time );
    static work_week current();
    work_week(const work_week & ) = default;

    unsigned get_year() const;
    unsigned get_work_week() const;
    int operator-( const work_week & ww ) const;
};

// Returns the number of work weeks since given time
unsigned get_work_weeks_since( const work_week & start );

}  // namespace time
}  // namespace utilities
