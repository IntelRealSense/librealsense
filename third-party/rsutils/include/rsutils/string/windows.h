// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <sstream>

#ifdef _WIN32
#include <Windows.h>

namespace rsutils {
namespace string {
namespace windows {


inline std::string win_to_utf( const WCHAR * s )
{
    int const wlen = -1;  // null terminated
    auto len = WideCharToMultiByte( CP_UTF8, 0, s, wlen, nullptr, 0, nullptr, nullptr );
    if( len == 0 )
    {
        std::ostringstream ss;
        ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
        throw std::runtime_error( ss.str() );
    }

    std::string buffer;
    buffer.resize( len - 1 );  // len includes the \0 only if wlen==-1!
    len = WideCharToMultiByte( CP_UTF8, 0, s, wlen, &buffer[0], len, nullptr, nullptr );
    if( len == 0 )
    {
        std::ostringstream ss;
        ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
        throw std::runtime_error( ss.str() );
    }

    return buffer;
}


inline std::string win_to_utf( std::wstring const & s )
{
    return win_to_utf( s.c_str() );
}


inline std::string win_to_utf( char const * s )
{
    return s;
}


inline std::string const & win_to_utf( std::string const & s )
{
    return s;
}


}  // namespace windows
}  // namespace string
}  // namespace rsutils
#endif