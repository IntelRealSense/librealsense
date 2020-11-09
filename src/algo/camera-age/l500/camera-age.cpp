//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "camera-age.h"
#include <string>


namespace librealsense {
namespace algo {
namespace camera_age {
namespace l500 {

work_week::work_week( std::time_t & t )
{
    auto time = std::localtime( t );
    int year = time->tm_year + 1900;  // The tm_year field contains the number of years
                                      // since 1900, we add 1900 to get current year
    int ww = time->tm_yday / 7;       // The tm_yday field contains the number of days sine 1st
                                      // of January, we divide by 7 to get current work week
    ww++;  // We add 1 because work weeks start at 1 and not 0 (ex. 1st of January will return
           // ww 0, but it ww 1)
    return work_week( year, ww );
}

unsigned work_week::get_year() const
{
    return man_year;
}

unsigned work_week::get_work_week() const
{
    return man_ww;
}

unsigned work_week::operator-( const work_week & ww ) const
{
    return ( ( this->get_year() - ww.get_year() ) * 52 )  // There are 52 weeks in a year
         + ( this->get_work_week() - ww.get_work_week() );
}
// The Serial Number format is PYWWXXXX:
// P – Site Name(ex.“F” for Fabrinet)
// Y – Year(ex.“9” for 2019, "0" for 2020, , "1" for 2021  ..etc)
// WW – Work Week
// XXXX – Sequential number
work_week get_manufature_work_week( const std::string & serial )
{
    if( serial.size() != 8 )
        throw invalid_value_exception( "invalid serial number: " + serial + " is of invalid size" );
    unsigned Y = serial[1] - '0';  // Converts char to int, '0'-> 0, '1'-> 1, ...
    unsigned man_year = 0;
    // using Y from serial number to get manufactoring year
    if( Y == 9 )
        man_year = 2019;
    else if( Y >= 0 && Y <= 8 )
        man_year = 2020 + Y;
    else
        throw invalid_value_exception( "invalid serial number: " + serial + " has invalid year" );
    // using WW from serial number to get manufactoring work week
    unsigned WW_tens = serial[2] - '0';
    unsigned WW_singles = serial[3] - '0';
    if( WW_tens > 9 || WW_tens < 0 || WW_singles > 9 || WW_singles < 0 )
        throw invalid_value_exception( "invalid serial number: " + serial
                                       + " has invalid work week" );
    unsigned man_ww = ( (WW_tens)*10 ) + WW_singles;
    return work_week( man_year, man_ww );
}

unsigned get_work_weeks_since( const work_week & start )
{
    auto t = ( std::time( nullptr ) );
    work_week now( t );
    unsigned age = now - start;
    return age;
}

}  // namespace l500
}  // namespace camera_age
}  // namespace algo
}  // namespace librealsense