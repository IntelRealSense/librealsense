// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <string.h>  // strcasecmp in linux

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif


namespace rsutils {
namespace string {


// Use in containers:
//      std::map< KEY, VALUE, nocase_less_t >
//
struct nocase_less_t
{
    // case-independent (ci) compare_less binary function
    bool operator()( std::string const & l, std::string const & r ) const
    {
        // If internationalization or embedded NULLs aren't an issue, this should be faster than the STL
        return strcasecmp( l.c_str(), r.c_str() ) < 0;
        //return std::lexicographical_compare( s1.begin(), s1.end(),
        //                                     s2.begin(), s2.end(),
        //                                     nocase_compare() );  // comparison
    }
};


inline int nocase_compare( std::string const & l, std::string const & r )
{
    return strcasecmp( l.c_str(), r.c_str() );
}
inline int nocase_compare( std::string const & l, char const * r )
{
    return strcasecmp( l.c_str(), r );
}
inline int nocase_compare( char const * l, std::string const & r )
{
    return strcasecmp( l, r.c_str() );
}
inline int nocase_compare( char const * l, char const * r )
{
    return strcasecmp( l, r );
}


inline int nocase_equal( std::string const & l, std::string const & r )
{
    return l.length() == r.length() && 0 == strcasecmp( l.c_str(), r.c_str() );
}
inline int nocase_equal( std::string const & l, char const * r )
{
    return 0 == strcasecmp( l.c_str(), r );
}
inline int nocase_equal( char const * l, std::string const & r )
{
    return 0 == strcasecmp( l, r.c_str() );
}
inline int nocase_equal( char const * l, char const * r )
{
    return 0 == strcasecmp( l, r );
}


}  // namespace string
}  // namespace rsutils
