// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <sstream>

#ifdef WIN32
#include <Windows.h>

namespace utilities {
namespace string {
namespace windows {


inline std::string win_to_utf( const WCHAR * s, size_t wlen = -1 )
{
    auto len = WideCharToMultiByte( CP_UTF8, 0, s, (int)wlen, nullptr, 0, nullptr, nullptr );
    if( len == 0 )
    {
        std::ostringstream ss;
        ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
        throw std::runtime_error( ss.str() );
    }

    std::string buffer;
    buffer.reserve( len );
    len = WideCharToMultiByte( CP_UTF8, 0, s, (int)wlen, &buffer[0], len, nullptr, nullptr );
    if( len == 0 )
    {
        std::ostringstream ss;
        ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
        throw std::runtime_error( ss.str() );
    }
    buffer.resize( len );

    return buffer;
}


inline std::string win_to_utf( std::wstring const & s )
{
    return win_to_utf( s.c_str(), s.length() );
}


}  // namespace windows
}  // namespace string
}  // namespace utilities
#endif