// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/string/shorten-json-string.h>


namespace rsutils {
namespace string {


// Given an outside block, e.g.:
//        012345678901234567890123456789012345678901234567890123456789012345678901234567890
//        0         1         2         3         4         5         6         7         8
//        {"one":1,"two":[1,2,3],"three":{"longassblock":{"insideblock":89012}},"four":4}
// Find the first inside block, including the enclosing delimiters (parenthesis or square brackets):
//                       ^______^        ^_____________________________________^
//
static slice find_inside_block( slice const & outside )
{
    // find an opening
    slice::const_iterator it = outside.begin() + 1;  // opening {, separating comma, etc.
    bool in_quote = false;
    while( it < outside.end() && ( in_quote || *it != '[' && *it != '{' ) )
    {
        if( *it == '"' )
            in_quote = ! in_quote;
        ++it;
    }
    if( it >= outside.end() )
        return slice();
    assert( ! in_quote );
    auto const begin = it;
    char const open = *begin;

    // find the closing
    char const close = ( *it == '[' ) ? ']' : '}';
    int nesting = 0;
    while( 1 )
    {
        if( ++it == outside.end() )
            // Something is invalid
            return slice();

        if( *it == '"' )
            in_quote = ! in_quote;
        else if( ! in_quote )
        {
            if( *it == close )
            {
                if( 0 == nesting )
                    break;
                --nesting;
            }
            else if( *it == open )
                ++nesting;
        }
    }
    return slice( begin, it + 1 );  // including the enclosing {} or []
}


// max-length output
// ---------- ---------------------------------------------------
//          7 { ... }
//          8 {" ... }
//          9 {"a ... }
//         22 {"a[1]":1,"b[2": ... }
//         29 {"a[1]":1,"b[2":3,"d":[ ... }
//         41 {"a[1]":1,"b[2":3,"d":[1,2,{3,4,5, ... ]}
//         42 {"a[1]":1,"b[2":3,"d":[1,2,{ ... },6,7,8]}
//         43 {"a[1]":1,"b[2":3,"d":[1,2,{3 ... },6,7,8]}
//         49 {"a[1]":1,"b[2":3,"d":[1,2,{3,4,5,6 ... },6,7,8]}
//         50 {"a[1]":1,"b[2":3,"d":[1,2,{3,4,5,6,7,8,9},6,7,8]}   <--   original json string
//
ellipsis shorten_json_string( slice const & str, size_t max_length )
{
    if( str.length() <= max_length )
        // Already short enough
        return ellipsis( str, slice() );
    if( max_length < 7 )
        return ellipsis( slice(), str );  // impossible: "{ ... }" is the minimum

    // Try to find an inside block that can be taken out
    ellipsis final;
    slice range = str;
    while( slice block = find_inside_block( range ) )
    {
        // Removing the whole block is one option:
        //        0123456789012345678901234567890123456789012345678
        //        0         1         2         3         4
        //        {"one":1,"two":[1,2,3],"three":{ ... },"four":4}
        //        ^_______________________________^    ^__________^
        ellipsis candidate( slice( str.begin(), block.begin() + 1 ), slice( block.end() - 1, str.end() ) );
        if( candidate.length() <= max_length && candidate.length() > final.length() )
            final = candidate;

        // But we might be able to remove only an inside block and get a better result:
        auto inside_max_length = int(max_length) - ( block.begin() - str.begin() ) - ( str.end() - block.end() );
        if( inside_max_length > 6 )
        {
            auto inside = shorten_json_string( block, inside_max_length );
            if( inside )
            {
                // See if shortening the inside block gives a better result than shortening
                // the outside...
                //        {"one":1,"two":[1,2,3],"three":{"longassblock":{ ... }},"four":4}
                //                                       ^________________^    ^_^
                ellipsis inside_candidate( slice( str.begin(), inside.first.end() ),
                                           slice( inside.second.begin(), str.end() ) );
                if( inside_candidate.length() <= max_length  &&  inside_candidate.length() > final.length() )
                    final = inside_candidate;
            }
        }

        // Next iteration, continue right after this block
        range = slice( block.end(), str.end() );
    }

    // The minimal solution is to shorten the current block at the end (if we can't find anything else)
    if( ! final )
        final = ellipsis( slice( str.begin(), str.begin() + max_length - 6 ),
                          slice( str.end() - 1, str.end() ) );
    return final;
}


}  // namespace string
}  // namespace rsutils
