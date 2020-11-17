//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "get-mfr-ww.h"
#include <string>


namespace utilities {
namespace time {
namespace l500 {
// The Serial Number format is PYWWXXXX:
// P – Site Name(ex.“F” for Fabrinet)
// Y – Year(ex.“9” for 2019, "0" for 2020, , "1" for 2021  ..etc)
// WW – Work Week
// XXXX – Sequential number
work_week get_manufacture_work_week( const std::string & serial )
{
    if( serial.size() != 8 )
        throw std::runtime_error( "Invalid serial number \"" + serial + "\" length" );
    unsigned Y = serial[1] - '0';  // Converts char to int, '0'-> 0, '1'-> 1, ...
    unsigned man_year = 0;
    // using Y from serial number to get manufactoring year
    if( Y == 9 )
        man_year = 2019;
    else if( Y < 9 )
        man_year = 2020 + Y;
    else
        throw std::runtime_error( "Invalid serial number \"" + serial + "\" year" );
    // using WW from serial number to get manufactoring work week
    unsigned WW_tens = serial[2] - '0';
    unsigned WW_singles = serial[3] - '0';
    if( WW_tens > 5 || WW_singles > 9 || ( WW_tens == 5 && WW_singles > 3 ) )
        throw std::runtime_error( "Invalid serial number \"" + serial + "\" work week" );
    unsigned man_ww = ( (WW_tens)*10 ) + WW_singles;
    return work_week( man_year, man_ww );
}

}  // namespace l500
}  // namespace time
}  // namespace utilities
