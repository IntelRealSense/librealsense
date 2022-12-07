// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>

namespace rsutils {
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
inline std::string trim_newlines( std::string text )
{
    char const * const base = text.c_str();  // first character in the text
    char * dest = (char *)base;              // running pointer to next destination character
    char const * src = dest;                 // running pointer to current source character
    //      "abc  \nline"
    //      dest-^  ^-src
    while( *src )
    {
        // Copy everything until we hit a newline
        if( *src != '\n' )
        {
            *dest++ = *src++;
            continue;
        }
        // Go back and remove spaces
        while( dest > base && dest[-1] == ' ' )
            --dest;
        // We're going to insert something (either space or newline); move src forward by one
        ++src;
        // If we're at the end of the string -- don't touch
        // If we're at another newline -- don't touch
        // If we're at the start of the text (dest==base) -- don't touch
        if( *src && *src != '\n' && dest > base )
        {
            // If previous line does not end with ':' and is not empty, insert a space
            if( dest[-1] != ':' && dest[-1] != '\n' )
            {
                *dest++ = ' ';
                continue;
            }
        }
        *dest++ = '\n';
    }
    text.resize( dest - base );
    return text;
}


}  // namespace string
}  // namespace rsutils
