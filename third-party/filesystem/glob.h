// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

#pragma once
#include <string>
#if defined _WIN32 || defined WINCE
# include <windows.h>
const char dir_separators[] = "/\\";

namespace
{
    struct dirent
    {
        const char* d_name;
    };

    struct DIR
    {
#if defined(WINRT) || defined(_WIN32_WCE)
        WIN32_FIND_DATAW data;
#else
        WIN32_FIND_DATAA data;
#endif
        HANDLE handle;
        dirent ent;
#ifdef WINRT
        DIR() { }
        ~DIR()
        {
            if( ent.d_name )
                delete[] ent.d_name;
        }
#endif
    };

    DIR* opendir( const char* path )
    {
        DIR* dir = new DIR;
        dir->ent.d_name = 0;
#if defined(WINRT) || defined(_WIN32_WCE)
        cv::String full_path = cv::String( path ) + "\\*";
        wchar_t wfull_path[MAX_PATH];
        size_t copied = mbstowcs( wfull_path, full_path.c_str(), MAX_PATH );
        CV_Assert( (copied != MAX_PATH) && (copied != (size_t)-1) );
        dir->handle = ::FindFirstFileExW( wfull_path, FindExInfoStandard,
            &dir->data, FindExSearchNameMatch, NULL, 0 );
#else
        dir->handle = ::FindFirstFileExA( (std::string( path ) + "\\*").c_str(),
            FindExInfoStandard, &dir->data, FindExSearchNameMatch, NULL, 0 );
#endif
        if( dir->handle == INVALID_HANDLE_VALUE )
        {
            /*closedir will do all cleanup*/
            delete dir;
            return 0;
        }
        return dir;
    }

    dirent* readdir( DIR* dir )
    {
#if defined(WINRT) || defined(_WIN32_WCE)
        if( dir->ent.d_name != 0 )
        {
            if( ::FindNextFileW( dir->handle, &dir->data ) != TRUE )
                return 0;
        }
        size_t asize = wcstombs( NULL, dir->data.cFileName, 0 );
        CV_Assert( (asize != 0) && (asize != (size_t)-1) );
        char* aname = new char[asize + 1];
        aname[asize] = 0;
        wcstombs( aname, dir->data.cFileName, asize );
        dir->ent.d_name = aname;
#else
        if( dir->ent.d_name != 0 )
        {
            if( ::FindNextFileA( dir->handle, &dir->data ) != TRUE )
                return 0;
        }
        dir->ent.d_name = dir->data.cFileName;
#endif
        return &dir->ent;
    }

    void closedir( DIR* dir )
    {
        ::FindClose( dir->handle );
        delete dir;
    }


}
#else
# include <dirent.h>
# include <sys/stat.h>
const char dir_separators[] = "/";
#endif


#ifdef _WIN32
static const char native_separator = '\\';
#else
static const char native_separator = '/';
#endif

static inline
bool isPathSeparator( char c )
{
    return c == '/' || c == '\\';
}

std::string join( const std::string& base, const std::string& path )
{
    if( base.empty() )
        return path;
    if( path.empty() )
        return base;

    bool baseSep = isPathSeparator( base[base.size() - 1] );
    bool pathSep = isPathSeparator( path[0] );
    std::string result;
    if( baseSep && pathSep )
    {
        result = base + path.substr( 1 );
    }
    else if( !baseSep && !pathSep )
    {
        result = base + native_separator + path;
    }
    else
    {
        result = base + path;
    }
    return result;
}

static bool wildcmp( const char *string, const char *wild )
{
    // Based on wildcmp written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
    const char *cp = 0, *mp = 0;

    while( (*string) && (*wild != '*') )
    {
        if( (*wild != *string) && (*wild != '?') )
        {
            return false;
        }

        wild++;
        string++;
    }

    while( *string )
    {
        if( *wild == '*' )
        {
            if( !*++wild )
            {
                return true;
            }

            mp = wild;
            cp = string + 1;
        }
        else if( (*wild == *string) || (*wild == '?') )
        {
            wild++;
            string++;
        }
        else
        {
            wild = mp;
            string = cp++;
        }
    }

    while( *wild == '*' )
    {
        wild++;
    }

    return *wild == 0;
}


static bool isDir( const std::string& path, DIR* dir )
{
#if defined _WIN32 || defined WINCE
    DWORD attributes;
    BOOL status = TRUE;
    if( dir )
        attributes = dir->data.dwFileAttributes;
    else
    {
        WIN32_FILE_ATTRIBUTE_DATA all_attrs;
#ifdef WINRT
        wchar_t wpath[MAX_PATH];
        size_t copied = mbstowcs( wpath, path.c_str(), MAX_PATH );
        CV_Assert( (copied != MAX_PATH) && (copied != (size_t)-1) );
        status = ::GetFileAttributesExW( wpath, GetFileExInfoStandard, &all_attrs );
#else
        status = ::GetFileAttributesExA( path.c_str(), GetFileExInfoStandard, &all_attrs );
#endif
        attributes = all_attrs.dwFileAttributes;
    }

    return status && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    (void)dir;  // avoid warnings about unused params
    struct stat stat_buf;
    if( 0 != stat( path.c_str(), &stat_buf ) )
        return false;
    int is_dir = S_ISDIR( stat_buf.st_mode );
    return is_dir != 0;
#endif
}

static void glob_rec( const std::string & directory,
    const std::string & wildchart,
    std::vector<std::string>& result,
    bool recursive,
    bool includeDirectories,
    const std::string & pathPrefix
)
{
    DIR *dir;

    if( (dir = opendir( directory.c_str() )) != 0 )
    {
        /* find all the files and directories within directory */
        try
        {
            struct dirent *ent;
            while( (ent = readdir( dir )) != 0 )
            {
                const char* name = ent->d_name;
                if( (name[0] == 0) || (name[0] == '.' && name[1] == 0) || (name[0] == '.' && name[1] == '.' && name[2] == 0) )
                    continue;

                std::string path = join( directory, name );
                std::string entry = join( pathPrefix, name );

                if( isDir( path, dir ) )
                {
                    if( recursive )
                        glob_rec( path, wildchart, result, recursive, includeDirectories, entry );
                    if( !includeDirectories )
                        continue;
                }

                if( wildchart.empty() || wildcmp( name, wildchart.c_str() ) )
                    result.push_back( entry );
            }
        }
        catch( ... )
        {
            closedir( dir );
            throw;
        }
        closedir( dir );
    }
    else
    {
        throw std::runtime_error( "could not open directory: " + directory );
    }
}

static void glob(
    const std::string & directory,
    const std::string & spec,
    std::function< void( std::string const & ) > fn,
    bool recursive = true,
    bool includeDirectories = false
)
{
    std::vector< std::string > results;
    glob_rec( directory, spec, results, recursive, includeDirectories, "" );
    for( auto r : results )
        fn( r );
}

static
std::string get_parent( std::string const & path, std::string * basename = nullptr )
{
    // Returns the parent and leaf for the given path:
    //     /foo/bar/  ->  '/foo/bar' and '' (empty)
    //     blah/..    ->  'blah'     and '..'
    auto x = path.find_last_of( dir_separators );
    if( x == std::string::npos )
        return std::string();
    if( basename )
        *basename = path.substr( x + 1 );
    return std::string( path, 0, x );
}
