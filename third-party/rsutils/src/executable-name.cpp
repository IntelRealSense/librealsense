// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/os/executable-name.h>

#if defined( PLATFORM_POSIX ) || defined( __linux__ )
#include <iostream>
#elif defined( _WIN32 )
#include <Windows.h>
#endif


namespace rsutils {
namespace os {


std::string executable_path()
{
#if defined( PLATFORM_POSIX ) || defined( __linux__ )

    std::string sp;
    std::ifstream( "/proc/self/comm" ) >> sp;
    return sp;

#elif defined( _WIN32 )

    char buf[MAX_PATH];
    GetModuleFileNameA( nullptr, buf, MAX_PATH );
    return buf;

#else

    static_assert(false, "unrecognized platform");

#endif
}


std::string executable_name( bool with_extension )
{
    auto path = executable_path();
    auto sep = path.find_last_of( "/\\" );
    if( sep != std::string::npos )
        path = path.substr( sep + 1 );
    if( ! with_extension )
    {
        auto period = path.find_last_of( '.' );
        if( period != std::string::npos )
            path.resize( period );
    }
    return path;
}


}  // namespace os
}  // namespace rsutils
