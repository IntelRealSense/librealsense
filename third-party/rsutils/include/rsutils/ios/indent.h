// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>
#include <iosfwd>


namespace rsutils {
namespace ios {


long & indent_flag( std::ios_base & );


inline long get_indent( std::ios_base & s ) { return indent_flag( s ); }
inline bool has_indent( std::ios_base & s ) { return get_indent( s ) > 0; }
inline void set_indent( std::ios_base & s, long indent ) { indent_flag( s ) = std::max( indent, 0L ); }
inline void add_indent( std::ios_base & s, long d ) { set_indent( s, get_indent( s ) + d ); }


// os << "struct" << ios::indent()
//    << field( "member1" ) << s.member1
//    << field( "member2" ) << s.member2
//    << ios::unindent();
struct indent
{
    int d;

    indent( int d_indent = 4 )
        : d( d_indent )
    {
    }
};
struct unindent : indent
{
    unindent( int d_indent = 4 )
        : indent( -d_indent )
    {
    }
};


std::ostream & operator<<( std::ostream &, indent const & );


}  // namespace ios
}  // namespace rsutils
