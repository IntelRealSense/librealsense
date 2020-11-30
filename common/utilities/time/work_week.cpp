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
    // Calculating the JDN (Julian Day Number) for the first day of the first work week of both years
    int Y = _year + 4799;
    int other_Y = other._year + 4799;
    int Jan_1_JDN = (365 * Y) + (Y / 4) - (Y / 100) + (Y / 400) - 31738;
    int other_Jan_1_JDN = (365 * other_Y) + (other_Y / 4) - (other_Y / 100) + (other_Y / 400) - 31738;
    int start_of_year_first_work_week = Jan_1_JDN - (Jan_1_JDN % 7);
    int start_of_other_year_first_work_week = other_Jan_1_JDN - (other_Jan_1_JDN % 7);

    // Subtracting the JDNs results in the number of days between 2 dates
    return ((start_of_other_year_first_work_week - start_of_year_first_work_week) / 7) + (_ww - other._ww);
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
