// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "filesystem.h"

#include <regex>
#include <cmath>


#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#include <KnownFolders.h>
#include <shlobj.h>
#endif

#if( defined( _WIN32 ) || defined( _WIN64 ) )
#include "ShellAPI.h"
#endif

#if defined __linux__ || defined __APPLE__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

namespace rs2 {

    
bool directory_exists( const char * dir )
{
    struct stat info;

    if( stat( dir, &info ) != 0 )
        return false;
    else if( info.st_mode & S_IFDIR )
        return true;
    else
        return false;
}


std::string get_file_name( const std::string & path )
{
    std::string file_name;
    for( auto rit = path.rbegin(); rit != path.rend(); ++rit )
    {
        if( *rit == '\\' || *rit == '/' )
            break;
        file_name += *rit;
    }
    std::reverse( file_name.begin(), file_name.end() );
    return file_name;
}


std::string get_timestamped_file_name()
{
    std::time_t now = std::time( NULL );
    std::tm * ptm = std::localtime( &now );
    char buffer[16];
    // Format: 20170529_205500
    std::strftime( buffer, 16, "%Y%m%d_%H%M%S", ptm );
    return buffer;
}


std::string get_folder_path( special_folder f )
{
    std::string res;
#ifdef _WIN32
    if( f == temp_folder )
    {
        TCHAR buf[MAX_PATH];
        if( GetTempPath( MAX_PATH, buf ) != 0 )
        {
            char str[1024];
            wcstombs( str, buf, 1023 );
            res = str;
        }
    }
    else
    {
        GUID folder;
        switch( f )
        {
        case user_desktop:
            folder = FOLDERID_Desktop;
            break;
        case user_documents:
            folder = FOLDERID_Documents;
            break;
        case user_pictures:
            folder = FOLDERID_Pictures;
            break;
        case user_videos:
            folder = FOLDERID_Videos;
            break;
        case app_data:
            folder = FOLDERID_RoamingAppData;
            break;
        default:
            throw std::invalid_argument( std::string( "Value of f (" ) + std::to_string( f )
                                         + std::string( ") is not supported" ) );
        }
        PWSTR folder_path = NULL;
        HRESULT hr = SHGetKnownFolderPath( folder, KF_FLAG_DEFAULT_PATH, NULL, &folder_path );
        if( SUCCEEDED( hr ) )
        {
            char str[1024];
            wcstombs( str, folder_path, 1023 );
            CoTaskMemFree( folder_path );
            res = str;
            res += "\\";
        }
        else
        {
            throw std::runtime_error( "Failed to get requested special folder" );
        }
    }
#endif  //_WIN32
#if defined __linux__ || defined __APPLE__
    if( f == special_folder::temp_folder )
    {
        const char * tmp_dir = getenv( "TMPDIR" );
        res = tmp_dir ? tmp_dir : "/tmp/";
    }
    else
    {
        const char * home_dir = getenv( "HOME" );
        if( ! home_dir )
        {
            struct passwd * pw = getpwuid( getuid() );
            home_dir = ( pw && pw->pw_dir ) ? pw->pw_dir : "";
        }
        if( home_dir )
        {
            res = home_dir;
            switch( f )
            {
            case user_desktop:
                res += "/Desktop/";
                break;
            case user_documents:
                res += "/Documents/";
                break;
            case user_pictures:
                res += "/Pictures/";
                break;
            case user_videos:
                res += "/Videos/";
                break;
            case app_data:
                res += "/.";
                break;
            default:
                throw std::invalid_argument( std::string( "Value of f (" ) + std::to_string( f )
                                             + std::string( ") is not supported" ) );
            }
        }
    }
#endif  // defined __linux__ || defined __APPLE__

    return res;
}


}  // namespace rs2
