// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/string/hexdump.h>
#include <ostream>
#include <iomanip>
#include <string>
#include <cstring>


namespace {


size_t _write( std::ostream & os, uint8_t const * const data, size_t cb )
{
    char const first_letter = ( os.flags() & std::ios::uppercase ) ? 'A' : 'a';
    uint8_t const * pb = data;
    while( cb-- > 0 )
    {
        uint8_t const hb = ( *pb >> 4 );
        os.put( char( hb > 9 ? ( hb - 10 + first_letter) : ( hb + '0' ) ) );
        uint8_t const lb = ( *pb & 0x0f );
        os.put( char( lb > 9 ? ( lb - 10 + first_letter ) : ( lb + '0' ) ) );
        ++pb;
    }
    return pb - data;
}


void _write_reverse( std::ostream & os, uint8_t const * const data, size_t cb, bool skip_leading_0s = false )
{
    if( skip_leading_0s )
        while( cb > 1 && ! data[cb - 1] )
            --cb;
    char const first_letter = ( os.flags() & std::ios::uppercase ) ? 'A' : 'a';
    while( cb-- > 0 )
    {
        uint8_t const hb = ( data[cb] >> 4 );
        if( ! skip_leading_0s || hb != 0 )
            os.put( char( hb > 9 ? ( hb - 10 + first_letter ) : ( hb + '0' ) ) );
        uint8_t const lb = ( data[cb] & 0x0f );
        os.put( char( lb > 9 ? ( lb - 10 + first_letter ) : ( lb + '0' ) ) );
        skip_leading_0s = false;
    }
}


unsigned _read_number( char const *& p_format, char const * const p_format_end )
{
    unsigned n = 0;
    while( p_format < p_format_end && *p_format >= '0' && *p_format <= '9' )
        n = n * 10 + (*p_format++ - '0');
    return n;
}


}


namespace rsutils {
namespace string {


std::ostream & operator<<( std::ostream & os, hexdump const & h )
{
    if( ! h._cb )
        return os;

    auto pb = h._data;
    size_t n_left = h._cb;
    if( h._max_bytes )
        n_left = std::min( h._max_bytes, n_left );
    if( ! h._gap )
    {
        if( h._big_endian )
            _write_reverse( os, pb, n_left );
        else
            _write( os, pb, n_left );
    }
    else
    {
        auto pend = pb + n_left;
        pb += _write( os, pb, std::min( h._gap, size_t( pend - pb ) ) );
        while( pb < pend )
        {
            os << h._gap_character;
            pb += _write( os, pb, std::min( h._gap, size_t( pend - pb ) ) );
        }
    }
    if( h._max_bytes && h._cb > h._max_bytes )
        os << "...";

    return os;
}


std::ostream & operator<<( std::ostream & os, hexdump::_format const & f )
{
    auto pb = f._h._data;
    size_t n_left = f._h._max_bytes ? f._h._max_bytes : f._h._cb;
    auto pend = pb + f._h._cb;
    char const * pf = f._fmt;
    char const * p_repeat = nullptr;  // set when we encounter {repeat:}
    unsigned c_repeats = 0;           // how many times to repeat; 0=infinite
    while( *pf )
    {
        if( *pf == '\\' )
        {
            if( pf[1] )
                ++pf;
        }
        else if( *pf == '{' )
        {
            char const * pfe = ++pf;
            int depth = 1;
            while( *pfe )
            {
                if( *pf == '\\' )
                {
                    if( pf[1] )
                        ++pf;
                }
                else if( *pfe == '}' )
                {
                    if( ! --depth )
                        break;
                }
                else if( *pfe == '{' )
                    ++depth;
                ++pfe;
            }
            if( *pfe )  // pfe now points to the closing '}'
            {
                size_t const directive_length = pfe - pf;
                // Grouping directives end with ':' (possibly followed by more information)
                if( ! p_repeat && 7 <= directive_length && 0 == strncmp( pf, "repeat:", 7 ) )
                {
                    pf += 7;  // past the ':'
                    c_repeats = _read_number( pf, pfe );
                    if( pf != pfe )
                        os << "{error:repeat:#}";
                    else
                        p_repeat = pfe + 1;
                    pf = pfe + 1;
                    continue;
                }
                // Group end is ':', possibly followed by more info
                if( *pf == ':' )
                {
                    // Close any previous grouping directive (right now, all we know is "repeat:")
                    if( p_repeat )
                    {
                        if( 1 == directive_length || pf[1] == '?' )
                        {
                            if( pb < pend )
                            {
                                // Haven't reached the end of the buffer
                                if( ! n_left )
                                {
                                    // Not allowed to output any more...
                                }
                                else if( c_repeats == 0 || --c_repeats > 0 )
                                {
                                    // Go back to the group beginning (right after the repeat:)
                                    pf = p_repeat;
                                    continue;
                                }
                                // done repeating
                                if( pf[1] == '?' )
                                {
                                    // "{:?...}" means print what follows (default to '...') if we still have more to go
                                    // E.g.:
                                    //      "dump[{2}{repeat:2} {2}{:?}]"
                                    // Will print the first 6 bytes of the buffer, with a gap every 2.
                                    // If there are more bytes then:
                                    //      "dump[0001 0203 0405...]"
                                    pf += 2;
                                    if( pf == pfe )
                                        os << "...";
                                    else
                                        os.write( pf, pfe - pf );
                                }
                            }
                            // done repeating; move past the directive
                            pf = pfe + 1;
                            continue;
                        }
                    }
                    os << "{error}";
                    pf = pfe + 1;
                    continue;
                }
                char const * const prefix = pf;
                if( *prefix == '+'       // "{+5}" means skip 5 bytes
                    || *prefix == '-' )  // "{-4}" -> in reverse (big-endian) byte order
                    ++pf;
                size_t n_bytes = _read_number( pf, pfe );
                if( *pf == 'i' || *pf == 'u' )
                {
                    if( pf + 1 != pfe )
                    {
                        os << "{error}";
                    }
                    else if( n_bytes != 1 && n_bytes != 2 && n_bytes != 4 && n_bytes != 8 )
                    {
                        os << "{error:1/2/4/8}";
                    }
                    else if( pb + n_bytes > pend )
                    {
                        os << "{error:not enough bytes}";
                    }
                    else
                    {
                        if( n_bytes == 1 )
                        {
                            if( *pf == 'i' )
                                os << std::to_string( int( *reinterpret_cast< int8_t const * >( pb ) ) );
                            else
                                os << std::to_string( int( *reinterpret_cast< uint8_t const * >( pb ) ) );
                        }
                        else if( n_bytes == 2 )
                        {
                            if( *pf == 'i' )
                                os << std::to_string( *reinterpret_cast< int16_t const * >( pb ) );
                            else
                                os << std::to_string( *reinterpret_cast< uint16_t const * >( pb ) );
                        }
                        else if( n_bytes == 4 )
                        {
                            if( *pf == 'i' )
                                os << std::to_string( *reinterpret_cast< int32_t const * >( pb ) );
                            else
                                os << std::to_string( *reinterpret_cast< uint32_t const * >( pb ) );
                        }
                        else if( n_bytes == 8 )
                        {
                            if( *pf == 'i' )
                                os << std::to_string( *reinterpret_cast< int64_t const * >( pb ) );
                            else
                                os << std::to_string( *reinterpret_cast< uint64_t const * >( pb ) );
                        }
                        pb += n_bytes;
                    }
                }
                else if( *pf == 'f' )
                {
                    if( pf + 1 != pfe )
                    {
                        os << "{error}";
                    }
                    else if( n_bytes != 4 && n_bytes != 8 )
                    {
                        os << "{error:4/8}";
                    }
                    else if( pb + n_bytes > pend )
                    {
                        os << "{error:not enough bytes}";
                    }
                    else
                    {
                        if( n_bytes == 4 )
                        {
                            os << std::to_string( *reinterpret_cast< float const * >( pb ) );
                        }
                        else if( n_bytes == 8 )
                        {
                            os << std::to_string( *reinterpret_cast< double const * >( pb ) );
                        }
                        pb += n_bytes;
                    }
                }
                else if( pf != pfe || ! n_bytes )
                {
                    os << "{error}";
                }
                else
                {
                    n_bytes = std::min( std::min( n_bytes, size_t( pend - pb ) ), n_left );
                    if( *prefix == '-' || *prefix == '0' )
                    {
                        _write_reverse( os, pb, n_bytes, *prefix == '0' || '0' == prefix[1] );
                        n_left -= n_bytes;  // skipped bytes are not output, therefore do not affect n_left
                    }
                    else if( *prefix != '+' )
                    {
                        _write( os, pb, n_bytes );
                        n_left -= n_bytes;  // skipped bytes are not output, therefore do not affect n_left
                    }
                    pb += n_bytes;
                }
                pf = pfe + 1;
                continue;
            }
        }
        os << *pf++;
    }

    return os;
}


}  // namespace string
}  // namespace rsutils
