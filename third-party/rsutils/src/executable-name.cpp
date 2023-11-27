// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/os/executable-name.h>

#if defined( PLATFORM_POSIX ) || defined( __linux__ )
#include <fstream>
#elif defined( _WIN32 )
#include <Windows.h>
#elif defined( __APPLE__ )
#include <mach-o/dyld.h>
#include <limits.h>  // PATH_MAX is POSIX, rather than MAXPATHLEN
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

#elif defined( __APPLE__ )

    // "With deep directories the total bufsize needed could be more than MAXPATHLEN."
    uint32_t const max_size = PATH_MAX + 1;
    char buf[max_size];
    uint32_t size = max_size;
    _NSGetExecutablePath( buf, &size );
    return buf;

#else

    static_assert( false, "unrecognized platform" );

#endif
}


std::string base_name( std::string path, bool with_extension )
{
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
