// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace utilities {
namespace string {


// Prepare a block of text for word-wrap by removing newlines:
//     - Remove trailing spaces from lines
//     - Multiple newlines stay as-is, so still separate paragraphs
//     - Newlines preceded by colons stay
//     - Newlines not preceded or followed by anything (beginning/end) stay
//     - All other newlines replaced by spaces
// Example: (\n added just for verbosity)
//     First line \n       // extra space removed
//     second line\n       // joined into first line
//     \n                  // stays
//     third line: \n      // trimmed; stays
//         fourth\n        // trailing \n stays
// Becomes:
//     First line second line\n
//     \n
//     third line:\n
//         fourth\n
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
