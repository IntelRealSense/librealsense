// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>
#include <ostream>
#include <type_traits>


namespace rsutils {
namespace string {


// Allow easy dumping of memory contents into a stream, so easily stringified
// 
// E.g.:
//     std::ostringstream ss;
//     ss << hexdump( this );  // output pointer to this object as a hex value
// 
// If you don't want a memory dump which may involve extra leading 0s but instead just to write a number in hex, use
// in_hex() below.
//
struct hexdump
{
    uint8_t const * const _data;
    size_t const _cb;

    size_t _max_bytes = 0;  // no more than this number of bytes (extra will print "..."; 0=no max)
    size_t _gap = 0;        // pad with spaces every <gap> bytes (0=no gap)
    char _gap_character = ' ';
    bool _big_endian = false;

    // Manual ctor for custom memory layout
    hexdump( uint8_t const * data, size_t len )
        : _data( data )
        , _cb( len )
    {
    }

    // Simplify usage with any type buffer, to avoid having to cast on the outside
    template< class T >
    hexdump( T const * pt, size_t len )
        : hexdump( reinterpret_cast< uint8_t const * >( pt ), sizeof( T ) )
    {
    }


    // Non-buffer usage would take the form of:
    //      hexdump( t )  // no length
    // These are easy to translate into a buffer.
    // However, if 't' is some human-legible type (integral, pointer), we make it so the values show big-endian:
    //      if i=0x01020304
    //      hexdump( &i, sizeof( i )) would give little-endian 04030201 <- not readable
    //      so hexdump( i ) should give 01020304

    template< typename T, typename = void >
    struct show_in_big_endian
    {
        static const bool value = false;
    };
    template< typename T >
    struct show_in_big_endian< T, std::enable_if_t< std::is_integral< T >::value || std::is_pointer< T >::value > >
    {
        static const bool value = true;
    };

    // Single-value dump: BIG-endian if integral or pointer; otherwise little-endian
    template< class T >
    hexdump( T const & t )
        : hexdump( &t, sizeof( T ) )
    {
        _big_endian = show_in_big_endian< T >::value;
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


// Write something out in hex without affecting the stream flags:
//
template< class T >
struct _in_hex
{
    T const & _t;

    _in_hex( T const & t )
        : _t( t )
    {
    }
};
template< class T >
inline std::ostream & operator<<( std::ostream & os, _in_hex< T > const & h )
{
    auto flags = os.flags();
    os << std::hex << h._t;
    os.flags( flags );
    return os;
}
template< class T >
inline _in_hex< T > in_hex( T const & t )
{
    return _in_hex< T >( t );
}


}  // namespace string
}  // namespace rsutils
