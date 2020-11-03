//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "camera-age.h"
#include <ctime>
#include <string>


namespace librealsense {
namespace algo {
namespace camera_age {
namespace l500 {

unsigned manufacture_time::get_manufacture_year() const
{
    return man_year;
}

unsigned manufacture_time::get_manufacture_work_week() const
{
    return man_ww;
}

// The Serial Number format is PYWWXXXX:
// P – Site Name(ex.“F” for Fabrinet)
// Y – Year(ex.“9” for 2019, "0" for 2020, , "1" for 2021  ..etc)
// WW – Work Week
// XXXX – Sequential number
manufacture_time get_manufature_time( const std::string & serial )
{
    unsigned Y = serial[1] - '0';  // Converts char to int, '0'-> 0, '1'-> 1, ...
    unsigned man_year = 0;
    // using Y from serial number to get manufactoring year
    if( Y == 9 )
        man_year = 2019;
    else
        man_year = 2020 + Y;
    // using WW from serial number to get manufactoring work week
    // unsigned man_ww = std::stoi( serial.substr( 2, 2 ) );
    unsigned man_ww = ( ( serial[2] - '0' ) * 10 ) + ( serial[3] - '0' );
    return manufacture_time( man_year, man_ww );
}

unsigned get_work_weeks_since( const manufacture_time & start )
{
    auto t = ( std::time( nullptr ) );
    auto curr_time = std::localtime( &t );
    int curr_year = curr_time->tm_year + 1900;  // The tm_year field contains the number of years
                                                // since 1900, we add 1900 to get current year
    int curr_ww = curr_time->tm_yday / 7;  // The tm_yday field contains the number of days sine 1st
                                           // of January, we divide by 7 to get current work week
    curr_ww++;  // We add 1 because work weeks start at 1 and not 0 (ex. 1st of January will return
                // ww 0, but it ww 1)
    unsigned age
        = ( ( curr_year - start.get_manufacture_year() ) * 52 )  // There are 52 weeks in a year
        + ( curr_ww - start.get_manufacture_work_week() );
    return age;
}

}  // namespace l500
}  // namespace camera_age
}  // namespace algo
}  // namespace librealsense