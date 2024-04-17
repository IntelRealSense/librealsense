// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <rsutils/ios/word-format.h>
#include <rsutils/string/from.h>
#include <cstring>


namespace rsutils {
namespace ios {


std::string word_format::str() const
{
    return rsutils::string::from( *this );
}


std::ostream & operator<<( std::ostream & os, word_format const & word )
{
    char prev_letter = 0;
    char prev_delim = 0;
    for( char const * pch = word._c_str; *pch; ++pch )
    {
        if( std::strchr( word._delimiters, *pch ) )
        {
            if( auto ch = word._delimiterizer( prev_delim, *pch ) )
                os << ch;
            prev_letter = 0;
            prev_delim = *pch;
        }
        else if( std::islower( prev_letter ) && std::isupper( *pch ) )
        {
            // Going from lower-case to upper-case counts as a delimiter followed by a new word
            // E.g.: leaseDuration -> lease duration
            if( auto ch = word._delimiterizer( 0, 0 ) )
                os << ch;
            if( auto ch = word._capitalizer( 0, prev_letter = *pch ) )
                os << ch;
        }
        else
        {
            if( auto ch = word._capitalizer( prev_letter, *pch ) )
                os << ch;
            prev_delim = 0;
            prev_letter = *pch;
        }
    }
    return os;
}


}  // namespace ios
}  // namespace rsutils
