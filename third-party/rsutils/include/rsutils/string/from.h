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

    // Specialty conversion: like std::to_string; fixed, high-precision (6)
    // Trims ending 0s and reverts to non-fixed notation if '0.' is the result...
    explicit from( double val, int precision = 6 );

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


inline std::ostream & operator<<( std::ostream & os, from const & f )
{
    // TODO c++20: use .rdbuf()->view()
    return os << f.str();
}


}  // namespace string
}  // namespace rsutils
