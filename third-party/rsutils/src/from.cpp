// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <rsutils/string/from.h>
#include <time.h>


namespace rsutils {
namespace string {


from::from( double d, int precision )
{
    char base[64];
    auto const len = std::snprintf( base, sizeof( base ), "%.*f", precision, d );
    if( len < 0 || len >= sizeof( base ) )
        _ss << d;
    else
    {
        char const * end = base + len;
        while( end > base && end[-1] == '0' )
            --end;
        if( *base == '0' && base[1] == '.' && end == base + 2 )
            _ss << d;
        else
            _ss.write( base, end - base );
    }
}


std::string from::datetime( tm const * time, char const * format )
{
    std::string buffer;
    if( time )
    {
        buffer.reserve( 50 );
        size_t cch = strftime( const_cast< char * >( buffer.data() ), buffer.capacity(), format, time );
        // "Provided that the result string, including the terminating null byte, does not exceed max bytes, strftime()
        // returns the number of bytes (excluding the terminating null byte) placed in the array. If the length of the
        // result string (including the terminating null byte) would exceed max bytes, then strftime() returns 0, and
        // the contents of the array are undefined."
        if( cch )
            buffer.resize( cch );
    }
    return buffer;
}


std::string from::datetime( char const * format )
{
    time_t t = time( nullptr );
    tm buf;
#ifdef WIN32
    localtime_s( &buf, &t );
    auto time = &buf;
#else
    auto time = localtime_r( &t, &buf );
#endif
    return datetime( time );
}


}  // namespace string
}  // namespace rsutils
