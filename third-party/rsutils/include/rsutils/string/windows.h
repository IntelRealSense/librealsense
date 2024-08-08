// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <sstream>

#ifdef WIN32
#include <Windows.h>

namespace rsutils {
namespace string {
namespace windows {


inline std::string win_to_utf( const WCHAR * s, int wlen = -1 )
{
    auto len = WideCharToMultiByte( CP_UTF8, 0, s, wlen, nullptr, 0, nullptr, nullptr );
    if( len == 0 )
    {
        std::ostringstream ss;
        ss << "WideCharToMultiByte(...) returned 0 and GetLastError() is " << GetLastError();
        throw std::runtime_error( ss.str() );
    }

    std::string buffer;
    buffer.resize( len - 1 );  // len includes the \0
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
    return win_to_utf( s.c_str(), (int) s.length() );
}


}  // namespace windows
}  // namespace string
}  // namespace rsutils
#endif