//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "work_week.h"
#include <string>


namespace utilities {
namespace time {

work_week::work_week( const std::time_t & t )
{
    auto time = std::localtime( &t );
    _year = time->tm_year + 1900;     // The tm_year field contains the number of years
                                      // since 1900, we add 1900 to get current year
    int Jan_1_wday = (time->tm_wday - time->tm_yday) % 7;
    if (Jan_1_wday < 0) 
        Jan_1_wday += 7;
    _ww = ((time->tm_yday + Jan_1_wday) / 7) + 1;
}

unsigned work_week::get_year() const
{
    return _year;
}

unsigned work_week::get_work_week() const
{
    return _ww;
}

int work_week::operator-( const work_week & other ) const
{
    auto age_in_weeks = (this->get_year() * 52) + this->get_work_week();
    auto other_age_in_weeks = (other.get_year() * 52) + other.get_work_week();
    return age_in_weeks - other_age_in_weeks;
}

work_week work_week::current()
{
    auto t = std::time( nullptr );
    return work_week( t );
}

unsigned get_work_weeks_since( const work_week & start )
{
    auto now = work_week::current();
    unsigned age = now - start;
    return age;
}

}  // namespace time
}  // namespace utilities
