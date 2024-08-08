// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/type/ip-address.h>
#include <rsutils/string/from.h>

#include <rsutils/json.h>
using rsutils::json;


namespace rsutils {
namespace type {


static bool parse_ip_part( char const *& pch, uint8_t & out_byte ) noexcept
{
    // Parse part of the IP address, and return it
    // Return false on failure
    // On success, pch should be on the character after the number:
    //        192.168.11.55
    //      in^  ^out

    if( *pch < '0' || *pch > '9' )
        return false;  // At least one digit
    
    int part = 0;
    do
    {
        part *= 10;
        part += *pch - '0';
        if( part > 255 )
            return false;  // Must be <= 255
        ++pch;
    }
    while( *pch >= '0' && *pch <= '9' );

    out_byte = (uint8_t)part;
    return true;
}


ip_address::ip_address( std::string const & str ) noexcept
    : ip_address()
{
    char const * pch = str.c_str();
    uint8_t b0, b1, b2, b3;
    if( parse_ip_part( pch, b0 ) && *pch == '.' && parse_ip_part( ++pch, b1 ) && *pch == '.'
        && parse_ip_part( ++pch, b2 ) && *pch == '.' && parse_ip_part( ++pch, b3 ) && ! *pch )
    {
        _b[0] = b0;
        _b[1] = b1;
        _b[2] = b2;
        _b[3] = b3;
    }
}


ip_address::ip_address( std::string const & str, _throw_if_not_valid )
    : ip_address( str )
{
    if( ! is_valid() )
        throw std::invalid_argument( "invalid IP address: " + str );
}


std::ostream & operator<<( std::ostream & os, ip_address const & ip )
{
    os << +ip._b[0] << '.';
    os << +ip._b[1] << '.';
    os << +ip._b[2] << '.';
    return os << +ip._b[3];
}


std::string ip_address::to_string() const
{
    return rsutils::string::from( *this );
}


void to_json( json & j, const ip_address & ip )
{
    j = ip.to_string();
}


void from_json( json const & j, ip_address & ip )
{
    if( ! j.is_string() )
        throw rsutils::json::type_error::create( 317, "ip_address should be a string", &j );

    ip_address tmp( j.string_ref() );
    if( ! tmp.is_valid() )
        throw rsutils::json::type_error::create( 317, "invalid ip_address", &j );

    ip = std::move( tmp );
}


}  // namespace type
}  // namespace rsutils