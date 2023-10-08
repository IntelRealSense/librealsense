// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/string/hexarray.h>
#include <rsutils/string/from.h>
#include <rsutils/string/slice.h>
#include <rsutils/string/hexdump.h>

#include <nlohmann/json.hpp>


namespace rsutils {
namespace string {


/*static*/ std::string hexarray::to_string( bytes const & bytearray )
{
    return rsutils::string::from() << hexdump( bytearray.data(), bytearray.size() );
}


/*static*/ std::ostream & hexarray::to_stream( std::ostream & os, bytes const & bytearray )
{
    return os << hexdump( bytearray.data(), bytearray.size() );
}


void to_json( nlohmann::json & j, const hexarray & hexa )
{
    j = hexa.to_string();
}


void from_json( nlohmann::json const & j, hexarray & hexa )
{
    if( j.is_array() )
    {
        hexa._bytes.resize( j.size() );
        std::transform( j.begin(), j.end(), std::begin( hexa._bytes ),
                        []( nlohmann::json const & elem )
                        {
                            if( ! elem.is_number_unsigned() )
                                throw nlohmann::json::type_error::create( 302, "array value not an unsigned integer", &elem );
                            auto v = elem.template get< uint64_t >();
                            if( v > 255 )
                                throw nlohmann::json::out_of_range::create( 401, "array value out of range", &elem );
                            return uint8_t( v );
                        } );
    }
    else if( j.is_string() )
        hexa = hexarray::from_string( j.get< std::string >() );
    else
        throw nlohmann::json::type_error::create( 317, "hexarray should be a string", &j );
}


hexarray hexarray::from_string( slice const & s )
{
    if( s.length() % 2 != 0 )
        throw std::runtime_error( "odd length" );
    char const * pch = s.begin();
    hexarray hexa;
    hexa._bytes.resize( s.length() / 2 );
    uint8_t * pb = hexa._bytes.data();
    while( pch < s.end() )
    {
        if( *pch >= '0' && *pch <= '9' )
            *pb = ( *pch - '0' ) << 4;
        else if( *pch >= 'a' && *pch <= 'f' )
            *pb = ( *pch - 'a' + 10 ) << 4;
        else
            throw std::runtime_error( "invalid character" );

        ++pch;
        if( *pch >= '0' && *pch <= '9' )
            *pb += ( *pch - '0' );
        else if( *pch >= 'a' && *pch <= 'f' )
            *pb += ( *pch - 'a' + 10 );
        else
            throw std::runtime_error( "invalid character" );
        ++pch;

        ++pb;
    }
    return hexa;
}


}  // namespace string
}  // namespace rsutils