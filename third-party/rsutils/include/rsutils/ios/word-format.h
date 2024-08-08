// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>
#include <iosfwd>


namespace rsutils {
namespace ios {


// Parse an input 0-terminated string into words, to output to a stream.
// 
// Words are delimited_by() any of a set of characters, which can then be removed or replaced by others as needed to
// provide custom output.
// 
// Custom capitalization can also be provided, with built-in uppercase, lower-case, or first-letter-only options.
// 
// E.g., with underscores:
//      MACRO_VARIABLE -> macro-variable   // replace _ with -, make lower-case
//      my_sharona -> My Sharona           // replace _ with space and capitalize first letters
// 
// By default:
//      - spaces and tabs are delimiters
//      - capitalization stays the same
//
struct word_format
{
    char const * const _c_str;
    char const * _delimiters;

    typedef char ( *replacement_function )( char previous, char current );

    replacement_function _delimiterizer = rf_same;  // can replace any delimiters with another character
    replacement_function _capitalizer = rf_same;    // can capitalize non-delimiters as needed

    word_format( char const * c_str, char const * delimiters = " \t" )
        : _c_str( c_str )
        , _delimiters( delimiters )
    {
    }
    word_format( std::string const & str, char const * delimiters = " \t" )
        : word_format( str.c_str(), delimiters )
    {
    }

    word_format & capitalize_with( replacement_function capitalizer )
    {
        _capitalizer = capitalizer;
        return *this;
    }
    word_format & capitalize_first_letter() { return capitalize_with( rf_cap_first ); }
    word_format & uppercase() { return capitalize_with( rf_upper ); }
    word_format & lowercase() { return capitalize_with( rf_lower ); }

    word_format & delimited_by( char const * delims, replacement_function delimiterizer = rf_same )
    {
        _delimiters = delims;
        _delimiterizer = delimiterizer;
        return *this;
    }

    std::string str() const;

    // Possible replacement functions
    static char rf_same( char, char ch ) { return ch; }
    static char rf_lower( char, char ch ) { return std::tolower( ch ); }
    static char rf_upper( char, char ch ) { return std::toupper( ch ); }
    static char rf_cap_first( char prev, char ch )
    {
        bool const is_first = ( prev == 0 );
        return is_first ? std::toupper( ch ) : std::tolower( ch );
    }
    static char rf_space( char, char ) { return ' '; }
    static char rf_dash( char, char ) { return '-'; }
};


std::ostream & operator<<( std::ostream & os, word_format const & );


// Convert names to our standard "dash" notation:
//      - delimited by whitespace, underscores, dashes
//      - replaced by dashes
//      - all lowercase
// E.g.:
//      SomethingNice -> something-nice
//      HA_HA -> ha-ha
//      My name -> my-name
// This is known as dash-case or kebab-case. We use the former because it's just easier to remember...
// https://stackoverflow.com/questions/11273282/whats-the-name-for-hyphen-separated-case
//
inline word_format dash_case( char const * c_str )
{
    return word_format( c_str ).lowercase().delimited_by( " _-\t", word_format::rf_dash );
}
inline word_format dash_case( std::string const & str )
{
    return dash_case( str.c_str() );
}


}  // namespace ios
}  // namespace rsutils
