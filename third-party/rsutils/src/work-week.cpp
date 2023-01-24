//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <rsutils/time/work-week.h>
#include <string>
#include <stdexcept>
#include <sstream>
#include <time.h>


static int work_weeks_between_years( unsigned year, unsigned other_year )
{
    unsigned Jan_1_JDN = rsutils::time::jdn( year, 1, 1 );
    unsigned other_Jan_1_JDN = rsutils::time::jdn( other_year, 1, 1 );
    // We need to compare between weeks, so we get the JDN of the Sunday of the wanted weeks
    // (JDN + 1) % 7 gives us the day of the week for the JDN. 0 -> Sun, 1 -> Mon ...
    int start_of_year_first_work_week = Jan_1_JDN - ( ( Jan_1_JDN + 1 ) % 7 );
    int start_of_other_year_first_work_week = other_Jan_1_JDN - ( ( other_Jan_1_JDN + 1 ) % 7 );
    // Subtracting the JDNs results in the number of days between 2 dates
    return ( ( start_of_year_first_work_week - start_of_other_year_first_work_week ) / 7 );
}

static unsigned days_in_month( unsigned year, unsigned month )
{
    if( month == 2 )
    {
        if( ( year % 400 == 0 ) || ( year % 4 == 0 && year % 100 != 0 ) )
            return 29;
        else
            return 28;
    }
    // Months with 30 days
    else if( month == 4 || month == 6 || month == 9 || month == 11 )
        return 30;
    else
        return 31;
}

namespace rsutils {
namespace time {

work_week::work_week( unsigned year, unsigned ww )
{
    if( ww == 0 || ww > unsigned(work_weeks_between_years(year + 1, year)))
    {
        std::ostringstream message;
        message << "Invalid work week given: " << year << " doesn't have a work week " << ww;
        throw std::runtime_error(message.str());
    }
    _year = year;
    _ww = ww;
}

work_week::work_week( const std::time_t & t )
{
    tm buf;
#ifdef WIN32
    localtime_s( &buf, &t );
    auto time = &buf;
#else
    auto time = localtime_r( &t, &buf );
#endif

    _year = time->tm_year + 1900;  // The tm_year field contains the number of years
                                   // since 1900, we add 1900 to get current year
    int Jan_1_wday = ( time->tm_wday - time->tm_yday ) % 7;
    if( Jan_1_wday < 0 )
        Jan_1_wday += 7;
    _ww = ( ( time->tm_yday + Jan_1_wday ) / 7 ) + 1;
    // As explained in the header file, the last few days of a year may belong to work week 1 of the
    // following year. This is the case if we are at the end of December, and the next year start
    // before the week ends
    if( _ww == 53 && ( 31 - time->tm_mday ) < ( 6 - time->tm_wday ) )
    {
        _year++;
        _ww = 1;
    }
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
    return work_weeks_between_years( _year, other._year ) + ( this->_ww - other._ww );
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

// Calculation is according to formula found in the link provided in the headet file
unsigned jdn( unsigned year, unsigned month, unsigned day )
{
    if (month == 0 || day == 0 || month > 12 || day > days_in_month(year, month))
    {
        std::ostringstream message;
        message << "Invalid date given: " << day << "/" << month << "/" << year;
        throw std::runtime_error(message.str());
    }
    return ( ( 1461 * ( year + 4800 + ( ( (int)month - 14 ) / 12 ) ) ) / 4 )
         + ( ( 367 * ( month - 2 - ( 12 * ( ( (int)month - 14 ) / 12 ) ) ) ) / 12 )
         - ( ( 3 * ( ( year + 4900 + ( ( (int)month - 14 ) / 12 ) ) / 100 ) ) / 4 ) + day - 32075;
}
}  // namespace time
}  // namespace rsutils
