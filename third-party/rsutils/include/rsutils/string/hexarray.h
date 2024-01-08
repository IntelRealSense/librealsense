// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>

#include <vector>
#include <string>
#include <cstdint>
#include <iosfwd>


namespace rsutils {
namespace string {


class slice;


// If a 'bytearray' is an array of bytes, then an array of hex representations is a 'hexarray'.
// 
// An object wrapping a vector of bytes, for writing to/reading from either ostream or json. With ostream, usage is
// standard via operator<<. For json, the type should be implicitly compatible with the various json::get<> variants
// through the to_json() and from_json() functions provided below.
// 
// The representation is like a hexdump, but here we go in both directions: either to-hex or from-hex.
// 
// Since we represent data that needs to be returned, we cannot simply refer to an existing set of bytes (like hexdump,
// for example) and instead must own it. Construction is always through moves to avoid implicit data copies. Static
// functions for to_string() and to_stream() are provided to deal with such data that cannot be moved.
//
class hexarray
{
    typedef std::vector< uint8_t > bytes;

private:
    bytes _bytes;

    friend void from_json( rsutils::json const &, hexarray & );

public:
    hexarray() = default;
    hexarray( hexarray && hexa ) = default;
    hexarray( bytes && bytearray )
        : _bytes( std::move( bytearray ) )
    {
    }

    // Only lower-case hex is understood: any non-lower-case hex characters will throw
    static hexarray from_string( slice const & );

    hexarray & operator=( hexarray && ) = default;

    bytes detach() { return std::move( _bytes ); }
    bytes const & get_bytes() const { return _bytes; }

    bool empty() const { return _bytes.empty(); }
    void clear() { _bytes.clear(); }

    std::string to_string() const { return to_string( _bytes ); }
    static std::string to_string( bytes const & );

    static std::ostream & to_stream( std::ostream &, bytes const & );
};


inline std::ostream & operator<<( std::ostream & os, hexarray const & hexa )
{
    return hexa.to_stream( os, hexa.get_bytes() );
}


// Allow j["key"] = hexarray( bytes );
void to_json( rsutils::json &, const hexarray & );
// Allow j.get< hexarray >();
void from_json( rsutils::json const &, hexarray & );
// See https://github.com/nlohmann/json#arbitrary-types-conversions


// Utility function for parsing hexarray strings to the corresponding bytes.
// Output in 'pb'.
// Returns the new 'pb', or null if an invalid character was encountered;
//
uint8_t * hex_to_bytes( char const * start, char const * end, uint8_t * pb );


}  // namespace string
}  // namespace rsutils
