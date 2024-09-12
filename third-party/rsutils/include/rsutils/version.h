// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <iosfwd>
#include <cstdint>

struct tm;


namespace rsutils {


// Software versions have four numeric components making up a "version string":
//     "MAJOR.MINOR.PATCH[.BUILD]"
// Note that the BUILD is optional...
//
struct version
{
    typedef uint16_t sub_type;
    typedef uint64_t number_type;

    number_type number;

    constexpr version()
        : number( 0 )
    {
    }

    explicit version( number_type n )
        : number( n )
    {
    }

    version( sub_type major, sub_type minor, sub_type patch, sub_type build );

    explicit version( const char * );
    explicit version( const std::string & str )
        : version( str.c_str() )
    {
    }

    bool is_valid() const { return( number != 0 ); }
    operator bool() const { return is_valid(); }

    void clear() { number = 0; }

    sub_type get_major() const { return sub_type( number >> ( 8 * 6 ) ); }
    sub_type get_minor() const { return sub_type( number >> ( 8 * 4 ) ); }
    sub_type get_patch() const { return sub_type( number >> ( 8 * 2 ) ); }
    sub_type get_build() const { return sub_type( number              ); }

    version without_build() const { return version( number & ~0xFFFFULL ); }

    bool operator<=( version const & other ) const { return number <= other.number; }
    bool operator==( version const & other ) const { return number == other.number; }
    bool operator>( version const & other ) const { return number > other.number; }
    bool operator!=( version const & other ) const { return number != other.number; }
    bool operator>=( version const & other ) const { return number >= other.number; }
    bool operator<( version const & other ) const { return number < other.number; }
    bool is_between( version const & from, version const & until ) const
    {
        return ( from <= *this ) && ( *this <= until );
    }

    std::string to_string() const;
    operator std::string() const { return to_string(); }
};


std::ostream & operator<<( std::ostream &, version const & );


}  // namespace rsutils
