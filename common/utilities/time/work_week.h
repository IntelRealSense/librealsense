// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <ctime>


namespace utileties {
namespace time {

class work_week
{
    unsigned _year;
    unsigned _ww;

public:
    work_week( unsigned year, unsigned ww )
        : _year( year )
        , _ww( ww )
    {
    }
    work_week( const std::time_t & time );

    unsigned get_year() const;
    unsigned get_work_week() const;
    unsigned operator-( const work_week & ww ) const;
    static work_week current();
};
// Returns the number of work weeks since given time
unsigned get_work_weeks_since( const work_week & start );

namespace l500 {
work_week get_manufature_work_week( const std::string & serial );
}

}  // namespace time
}  // namespace utileties
