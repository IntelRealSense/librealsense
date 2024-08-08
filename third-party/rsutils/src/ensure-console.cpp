// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#if defined( WIN32 )
#include <iostream>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/os/hresult.h>
#endif


static void reopen_console_streams()
{
#if defined( WIN32 )
    // Need to re-open the standard streams if we have a console attached
    if( GetStdHandle( STD_OUTPUT_HANDLE ) )
    {
        // see https://stackoverflow.com/questions/15543571/allocconsole-not-displaying-cout
        FILE * fDummy;
        freopen_s( &fDummy, "CONOUT$", "w", stdout );
        freopen_s( &fDummy, "CONOUT$", "w", stderr );
        // Sync std::cout/cerr with stdout/err
        std::ios::sync_with_stdio();
        // Make sure they're clear of any error bits
        std::cout.clear();
        std::cerr.clear();
    }
#endif
}


namespace rsutils {
namespace os {


void ensure_console( bool create_if_none )
{
#if defined( WIN32 )
    if( AttachConsole( ATTACH_PARENT_PROCESS ) )
    {
        reopen_console_streams();
    }
    else
    {
        HRESULT hr = GetLastError();
        if( ERROR_ACCESS_DENIED == hr )
        {
            // "If the calling process is already attached to a console"
        }
        else if( ERROR_INVALID_HANDLE == hr || ERROR_INVALID_PARAMETER == hr )
        {
            // "If the specified process does not have a console"
            // "If the specified process does not exist"
            if( ! create_if_none )
            {
                LOG_DEBUG( "parent process has no console; none created" );
            }
            else if( AllocConsole() )
            {
                reopen_console_streams();
            }
            else
            {
                LOG_ERROR( "Parent process has no console; failed AllocConsole - "
                           << rsutils::hresult::hr_to_string( GetLastError() ) );
            }
        }
        else
        {
            LOG_ERROR( "failed AttachConsole - " << rsutils::hresult::hr_to_string( GetLastError() ) );
        }
    }
#endif
}


}  // namespace os
}  // namespace rsutils
