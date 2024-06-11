// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/string/ip-address.h>

#include <rsutils/json.h>
using rsutils::json;


namespace rsutils {
namespace string {


static int parse_ip_part( char const *& pch )
{
    // Parse part of the IP address, and return it
    // Return -1 on failure
    // On success, pch should be on the character after the number:
    //        192.168.11.55
    //      in^  ^out

    if( *pch < '0' || *pch > '9' )
        return -1;  // At least one digit
    
    int part = 0;
    do
    {
        part *= 10;
        part += *pch - '0';
        if( part > 255 )
            return -1;  // Must be <= 255
        ++pch;
    }
    while( *pch >= '0' && *pch <= '9' );

    return part;
}


ip_address::ip_address( std::string str )
{
    char const * pch = str.c_str();
    if( parse_ip_part( pch ) < 0 )
        return;
    if( *pch++ != '.' )
        return;
    if( parse_ip_part( pch ) < 0 )
        return;
    if( *pch++ != '.' )
        return;
    if( parse_ip_part( pch ) < 0 )
        return;
    if( *pch++ != '.' )
        return;
    if( parse_ip_part( pch ) < 0 )
        return;
    if( *pch )
        return;
    _str = std::move( str );
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


}  // namespace string
}  // namespace rsutils