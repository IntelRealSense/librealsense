// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace utilities {
namespace string {


inline std::string trim_newlines( std::string s )
{
    char const * const base = s.c_str();
    char * dest = (char *)base;
    char const * src = dest;
    while( *src )
    {
        if( *src != '\n' )
        {
            *dest++ = *src++;
            continue;
        }
        while( dest > base && dest[-1] == ' ' )
            --dest;
        ++src;
        if( *src && *src != '\n' && dest > base )
        {
            if( dest[-1] != ':' && dest[-1] != '\n' )
            {
                *dest++ = ' ';
                continue;
            }
        }
        *dest++ = '\n';
    }
    s.resize( dest - base );
    return s;
}


}  // namespace string
}  // namespace utilities
