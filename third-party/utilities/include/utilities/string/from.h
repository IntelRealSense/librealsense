// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <sstream>


struct tm;


namespace rsutils {
namespace string {


// Build a stringstream and automatically convert to string, all in one line.
//
struct from
{
    std::ostringstream _ss;

    from() = default;

    template< class T >
    explicit from( const T & val )
    {
        _ss << val;
    }

    template< class T >
    from & operator<<( const T & val )
    {
        _ss << val;
        return *this;
    }

    std::string str() const { return _ss.str(); }
    operator std::string() const { return _ss.str(); }

    // Returns an empty string if 'time' is null or the format requires too many characters.
    // See strftime for format specifiers.
    static std::string datetime( tm const * time, char const * format = "%Y-%m-%d-%H_%M_%S" );
    static std::string datetime( char const * format = "%Y-%m-%d-%H_%M_%S" );
};


}  // namespace string
}  // namespace rsutils
