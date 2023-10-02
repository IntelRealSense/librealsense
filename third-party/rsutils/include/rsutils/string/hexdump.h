// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>
#include <iosfwd>


namespace rsutils {
namespace string {


// Allow easy dumping of memory contents into a stream, so easily stringified
// 
// E.g.:
//     std::ostringstream ss;
//     ss << hexdump( this );  // output pointer to this object as a hex value
//
struct hexdump
{
    uint8_t const * const _data;
    size_t const _cb;

    size_t _max_bytes = 0;  // no more than this number of bytes (extra will print "..."; 0=no max)
    size_t _gap = 0;        // pad with spaces every <gap> bytes (0=no gap)
    char _gap_character = ' ';

    // Manual ctor for custom memory layout
    hexdump( uint8_t const * data, size_t len )
        : _data( data )
        , _cb( len )
    {
    }

    // Auto ptr for easy object dumping
    template< class T >
    hexdump( T const & t )
        : hexdump( reinterpret_cast< uint8_t const * >( &t ), sizeof( T ) )
    {
    }

    // Allow no more than this number of bytes out. If 0 (default), there is no maximum.
    // Anything past the max bytes will cause a '...' to be appended.
    //
    // E.g.:
    //      ss << hexdump( buffer, 100 ).max_bytes( 2 );
    // Will stream:
    //      0001...
    //
    hexdump & max_bytes( size_t cb )
    {
        _max_bytes = cb;
        return *this;
    }

    // Automatically insert a gap character (default ' ') every <gap> bytes, or none if 0.
    //
    // E.g.:
    //      ss << hexdump( int32_var ).gap( 2 );
    // Will stream:
    //      0001 0203
    //
    hexdump & gap( size_t cb, char character = ' ' )
    {
        _gap = cb;
        _gap_character = character;
        return *this;
    }

    struct _format
    {
        hexdump & _h;
        char const * _fmt;
    };

    // Allow custom formatting of the memory buffer.
    //
    // E.g.:
    //      ss << hexdump( vec.data(), vec.size() ).format( "two: {2} four: {4} two: {2}" );
    // Will stream the first 6 bytes:
    //      two: 0102 four: 03040506 two: 0708
    // 
    // Syntax:
    //      {#}            Output # bytes consecutively
    //      {0#}           Output # bytes, but with leading 0's removed (implies big-endian)
    //      {-[0]#}        Output # bytes, but in reverse (big-endian) order
    //      {#i}           Interpret # bytes as a signed integral value, and output decimal value
    //      {#u}           Interpret # bytes as an unsigned integral value, and output decimal value
    //      {#f}           Interpret # bytes as a floating point value (4f=float; 8f=double)
    //      {+#}           Skip # bytes
    //      \{             Output '{' (or any character following the '\')
    //      {repeat:#}     Start a repeat sequence that repeats # times (default is 0: as many times as buffer allows)
    //      {:}            End the current sequence and repeat it as necessary
    //      {:?}           Same, but add '...' if there are still more bytes
    //      {:?<text>}     Same, but add <text> if there are still more bytes
    //          E.g., a simple implementation of a gap:
    //              ss << hexdump(...).format( "data[{2}{repeat:} {2}{:?}]" );
    //
    inline _format format( char const * fmt )
    {
        return _format{ *this, fmt };
    }
};

std::ostream & operator<<( std::ostream & os, hexdump const & );
std::ostream & operator<<( std::ostream & os, hexdump::_format const & );


}  // namespace string
}  // namespace rsutils
