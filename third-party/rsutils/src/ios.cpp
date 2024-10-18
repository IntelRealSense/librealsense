// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/ios/field.h>
#include <rsutils/ios/indent.h>
#include <iostream>


namespace rsutils {
namespace ios {


long & indent_flag( std::ios_base & s )
{
    static int const index = std::ios_base::xalloc();
    return s.iword( index );
}


std::ostream & operator<<( std::ostream & os, indent const & indent )
{
    add_indent( os, indent.d );
    return os;
}


/*static*/ std::ostream & field::sameline( std::ostream & os )
{
    return os << ' ';
}


/*static*/ std::ostream & field::separator( std::ostream & os )
{
    if( int i = get_indent( os ) )
    {
        os << '\n';
        while( i-- )
            os << ' ';
    }
    else
    {
        sameline( os );
    }
    return os;
}


/*static*/ std::ostream & field::value( std::ostream & os )
{
    os << ' ';
    return os;
}


std::ostream & operator<<( std::ostream & os, field::group const & group )
{
    if( has_indent( os ) )
        os << indent();
    else
        os << "[";
    group.pos = &os;
    return os;
}


field::group::~group()
{
    if( pos )
    {
        if( has_indent( *pos ) )
            *pos << unindent();
        else
            *pos << " ]";
    }
}


}
}  // namespace rsutils
