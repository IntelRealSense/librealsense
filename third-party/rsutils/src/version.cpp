// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <rsutils/version.h>
#include <rsutils/string/from.h>


namespace rsutils {


version::version( sub_type m, sub_type n, sub_type p, sub_type b )
    : version( ( ( m & 0xFFULL ) << ( 8 * 7 ) ) + ( ( n & 0xFFFFULL ) << ( 8 * 5 ) )
             + ( ( p & 0xFFULL ) << ( 8 * 4 ) ) +   ( b & 0xFFFFFFFFULL ) )
{
    // Invalidate if any overflow
    if( m != get_major() )
        number = 0;
    else if( n != get_minor() )
        number = 0;
    else if( p != get_patch() )
        number = 0;
    else if( b != get_build() )
        number = 0;
}


version::version( char const * base )
    : version()
{
    if( ! base )
        return;
    unsigned major = 0;
    char const * ptr = base;
    while( *ptr != '.' )
    {
        if( *ptr < '0' || *ptr > '9' )
            return;  // If 0, unexpected; otherwise invalid
        major *= 10;
        major += *ptr - '0';
        if( major > 0xFF )
            return;  // Overflow
        ++ptr;
    }
    if( ptr == base )
        return;  // No major

    unsigned minor = 0;
    base = ++ptr;
    while( *ptr != '.' )
    {
        if( *ptr < '0' || *ptr > '9' )
            return;  // If 0, unexpected; otherwise invalid
        minor *= 10;
        minor += *ptr - '0';
        if( minor > 0xFFFF )
            return;  // Overflow
        ++ptr;
    }
    if( ptr == base )
        return;  // No minor

    unsigned patch = 0;
    base = ++ptr;
    while( *ptr != '.' )
    {
        if( ! *ptr )
            break;  // Acceptable: no build
        if( *ptr < '0' || *ptr > '9' )
            return;  // Invalid
        patch *= 10;
        patch += *ptr - '0';
        if( patch > 0xFF )
            return;  // Overflow
        ++ptr;
    }
    if( ptr == base )
        return;  // No patch

    uint64_t build = 0;
    if( *ptr )
    {
        base = ++ptr;
        while( *ptr )
        {
            if( *ptr < '0' || *ptr > '9' )
                return;  // Invalid
            build *= 10;
            build += *ptr - '0';
            if( build > 0xFFFFFFFFULL )
                return;  // Overflow
            ++ptr;
        }
        if( ptr == base )
            return;  // No build, but there was a period at the end...!?
    }

    number = version( major, minor, patch, (unsigned)build ).number;
}


std::string version::to_string() const
{
    return string::from() << *this;
}


std::ostream & operator<<( std::ostream & os, version const & v )
{
    // NOTE: FW versions were padded to 2 characters with leading 0s:
    //      os << std::setfill('0') << std::setw(2) << v.major() << '.' ...
    os << v.get_major() << '.' << v.get_minor() << '.' << v.get_patch();
    if( v.get_build() )
        os << '.' << v.get_build();
    return os;
}


}  // namespace rsutils
