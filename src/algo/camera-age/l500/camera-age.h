// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <ctime>


namespace librealsense {
namespace algo {
namespace camera_age {
namespace l500 {

class work_week
{
    unsigned year;
    unsigned ww;

public:
    work_week( unsigned man_year, unsigned man_ww )
        : year( man_year )
        , ww( man_ww )
    {
    }
    work_week( const std::time_t& time );

    unsigned get_year() const;
    unsigned get_work_week() const;
    unsigned operator-( const work_week & ww ) const;
};

work_week get_manufature_work_week( const std::string & serial );

// Returns the number of work weeks since given time
unsigned get_work_weeks_since( const work_week & start );

}  // namespace l500
}  // namespace camera_age
}  // namespace algo
}  // namespace librealsense
