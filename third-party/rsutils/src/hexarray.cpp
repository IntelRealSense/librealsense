// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/string/hexarray.h>
#include <rsutils/string/from.h>
#include <rsutils/string/slice.h>
#include <rsutils/string/hexdump.h>

#include <rsutils/json.h>


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


void to_json( rsutils::json & j, const hexarray & hexa )
{
    j = hexa.to_string();
}


void from_json( rsutils::json const & j, hexarray & hexa )
{
    if( j.is_array() )
    {
        hexa._bytes.resize( j.size() );
        std::transform( j.begin(), j.end(), std::begin( hexa._bytes ),
                        []( rsutils::json const & elem )
                        {
                            if( ! elem.is_number_unsigned() )
                                throw rsutils::json::type_error::create( 302, "array value not an unsigned integer", &elem );
                            auto v = elem.template get< uint64_t >();
                            if( v > 255 )
                                throw rsutils::json::out_of_range::create( 401, "array value out of range", &elem );
                            return uint8_t( v );
                        } );
    }
    else if( j.is_string() )
        hexa = hexarray::from_string( j.string_ref() );
    else
        throw rsutils::json::type_error::create( 317, "hexarray should be a string", &j );
}


uint8_t * hex_to_bytes( char const * pch, char const * const end, uint8_t * pb )
{
    while( pch < end )
    {
        if( *pch >= '0' && *pch <= '9' )
            *pb = (*pch - '0') << 4;
        else if( *pch >= 'a' && *pch <= 'f' )
            *pb = (*pch - 'a' + 10) << 4;
        else if( *pch >= 'A' && *pch <= 'F' )
            *pb = (*pch - 'A' + 10) << 4;
        else
            return nullptr;
        ++pch;

        if( *pch >= '0' && *pch <= '9' )
            *pb += (*pch - '0');
        else if( *pch >= 'a' && *pch <= 'f' )
            *pb += (*pch - 'a' + 10);
        else if( *pch >= 'A' && *pch <= 'F' )
            *pb += (*pch - 'A' + 10);
        else
            return nullptr;
        ++pch;

        ++pb;
    }
    return pb;
}


hexarray hexarray::from_string( slice const & s )
{
    if( s.length() % 2 != 0 )
        throw std::runtime_error( "odd length" );
    char const * pch = s.begin();
    hexarray hexa;
    hexa._bytes.resize( s.length() / 2 );
    uint8_t * pb = hexa._bytes.data();
    if( pb && ! hex_to_bytes( pch, s.end(), pb ) )
        throw std::runtime_error( "invalid character" );
    return hexa;
}


}  // namespace string
}  // namespace rsutils