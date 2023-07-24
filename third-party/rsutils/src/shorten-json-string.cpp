// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <rsutils/string/shorten-json-string.h>


namespace rsutils {
namespace string {


// Return the closing character corresponding to the opening passed in, or 0 otherwise
static char get_block_close( char const ch )
{
    if( ch == '[' )
        return ']';
    if( ch == '{' )
        return '}';
    if( ch == '"' )
        return '"';
    return 0;
}


static bool is_block_open( char const ch )
{
    return 0 != get_block_close( ch );
}


// Given an outside block, e.g.:
//        012345678901234567890123456789012345678901234567890123456789012345678901234567890
//        0         1         2         3         4         5         6         7         8
//        {"one":1,"two":[1,2,3],"three":{"longassblock":{"insideblock":89012}},"four":4}
// Find the first inside block, including the enclosing delimiters (parenthesis or square brackets):
//                       ^______^        ^_____________________________________^
// 
// @param target_item_it
//      if p_array_item is not null, points to a place inside the block for which we want to find the item that contains
//      it: the item is the array element (so comma-delimited) inside the block that spans this place.
//      E.g.:                [1,"2 [ ha ha to 3",3]
//                              |---------^----| target_item_it
//      the array element is    |     here     |
// @param p_array_item
//      on output will point to the beginning of the array element identified by target_item_it - must not be nullptr if
//      target_item_it is not nullptr!
//
static slice find_inside_block( slice const & outside,
                                slice::const_iterator const target_item_it = nullptr,
                                slice::const_iterator * p_array_item = nullptr )
{
    assert( ! target_item_it || p_array_item );

    // find an opening
    slice::const_iterator it = outside.begin() + 1;  // opening {, separating comma, etc.
    if( it <= target_item_it )
        *p_array_item = it;
    while( it < outside.end() && ! is_block_open( *it ) )
    {
        if( *it == ',' && it < target_item_it )
            *p_array_item = it + 1;
        ++it;
    }
    if( it >= outside.end() )
        return slice();
    auto const begin = it;
    char const open = *begin;

    // find the closing
    char const close = get_block_close( *it );
    int nesting = 0;
    bool in_quote = false;
    while( 1 )
    {
        if( ++it == outside.end() )
            // Something is invalid
            return slice();

        if( close != '"' && *it == '"' )
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
ellipsis shorten_json_string( slice const & str, size_t const max_length )
{
    if( str.length() <= max_length )
        // Already short enough
        return ellipsis( str, slice() );
    if( max_length < 7 )
        return ellipsis( slice(), str );  // impossible: "{ ... }" is the minimum

    // Try to find an inside block that can be taken out
    ellipsis result;
    slice range = str;
    slice::const_iterator const last_possible_break = str.begin() + max_length - 6;
    slice::const_iterator last_item_break = last_possible_break;
    while( slice block = find_inside_block( range, last_possible_break, &last_item_break ) )
    {
        // Removing the whole block is one option:
        //        0123456789012345678901234567890123456789012345678
        //        0         1         2         3         4
        //        {"one":1,"two":[1,2,3],"three":{ ... },"four":4}
        //        ^_______________________________^    ^__________^
        ellipsis candidate( slice( str.begin(), block.begin() + 1 ), slice( block.end() - 1, str.end() ) );
        if( candidate.length() <= max_length && candidate.length() > result.length() )
            result = candidate;

        // But we might be able to remove only an inside block and get a better result:
        auto inside_max_length = int( max_length ) - ( block.begin() - str.begin() ) - ( str.end() - block.end() );
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
                if( inside_candidate.length() <= max_length && inside_candidate.length() > result.length() )
                    result = inside_candidate;
            }
        }

        // Next iteration, continue right after this block
        range = slice( block.end(), str.end() );
    }

    // The minimal solution is to shorten the current block at the end (if we can't find anything else)
    if( ! result )
        result = ellipsis( slice( str.begin(), last_item_break ), slice( str.end() - 1, str.end() ) );
    return result;
}


}  // namespace string
}  // namespace rsutils
