// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

// This is in lieu of the C++20 chrono_io.h

// NOTE: make sure to include this file BEFORE any usage in another header
// (e.g., Catch should be include after this! Without these definitions, Catch
// output does not know how to stringify durations...)

#pragma once

#include <chrono>
#include <string>
#include <sstream>
#include <ctime>


inline std::string to_string( const std::time_t & time )
{
    std::ostringstream os;
    os << time;
    return os.str();
}


template< typename Clock, typename Duration = typename Clock::duration >
std::string to_string( const std::chrono::time_point< Clock, Duration > & tp )
{
    auto in_time_t = std::chrono::system_clock::to_time_t( tp );
    return to_string( in_time_t );
}


template< typename Rep, typename Period = std::ratio< 1 > >
std::string to_string( const std::chrono::duration< Rep, Period > & duration )
{
    auto seconds_as_int = std::chrono::duration_cast< std::chrono::seconds >( duration );
    if( seconds_as_int == duration )
        return std::to_string( seconds_as_int.count() ) + "s";
    auto seconds_as_double = std::chrono::duration_cast< std::chrono::duration< double > >( duration );
    std::ostringstream os;
    os << seconds_as_double.count();
    os << 's';
    return os.str();
}


template< typename Clock, typename Duration = typename Clock::duration >
std::ostream & operator<<( std::ostream & o, const std::chrono::time_point< Clock, Duration > & tp )
{
    return o << to_string( tp );
}


template< typename Rep, typename Period = std::ratio< 1 > >
std::ostream & operator<<( std::ostream & o, const std::chrono::duration< Rep, Period > & duration )
{
    return o << to_string( duration );
}
