//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "work_week.h"
#include <string>
#include <types.h>


namespace utilities {
namespace time {

work_week::work_week( const std::time_t & t )
{
    auto time = std::localtime( &t );
    _year = time->tm_year + 1900;     // The tm_year field contains the number of years
                                      // since 1900, we add 1900 to get current year
    _ww = ( time->tm_yday / 7 ) + 1;  // The tm_yday field contains the number of days sine 1st
                                      // of January, we divide by 7 to get current work week
                                      // than add 1 because work weeks start at 1 and not 0
                                      // (ex. 1st of January will return ww 0, but it ww 1)
}

unsigned work_week::get_year() const
{
    return _year;
}

unsigned work_week::get_work_week() const
{
    return _ww;
}

unsigned work_week::operator-( const work_week & ww ) const
{
    return ( ( this->get_year() - ww.get_year() ) * 52 )  // There are 52 weeks in a year
         + ( this->get_work_week() - ww.get_work_week() );
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

namespace l500 {
// The Serial Number format is PYWWXXXX:
// P – Site Name(ex.“F” for Fabrinet)
// Y – Year(ex.“9” for 2019, "0" for 2020, , "1" for 2021  ..etc)
// WW – Work Week
// XXXX – Sequential number
work_week get_manufacture_work_week( const std::string & serial )
{
    if( serial.size() != 8 )
        throw std::runtime_error( "Invalid serial number \"" + serial
                                                     + "\" length" );
    unsigned Y = serial[1] - '0';  // Converts char to int, '0'-> 0, '1'-> 1, ...
    unsigned man_year = 0;
    // using Y from serial number to get manufactoring year
    if( Y == 9 )
        man_year = 2019;
    else if( Y < 9 )
        man_year = 2020 + Y;
    else
        throw std::runtime_error( "Invalid serial number \"" + serial
                                                     + "\" year" );
    // using WW from serial number to get manufactoring work week
    unsigned WW_tens = serial[2] - '0';
    unsigned WW_singles = serial[3] - '0';
    if( WW_tens > 5 || WW_singles > 9 || ( WW_tens == 5 && WW_singles > 3))
        throw std::runtime_error( "Invalid serial number \"" + serial
                                                     + "\" work week" );
    unsigned man_ww = ( (WW_tens)*10 ) + WW_singles;
    return work_week( man_year, man_ww );
}
}  // namespace l500

}  // namespace time
}  // namespace utilities
