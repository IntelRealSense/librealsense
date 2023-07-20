// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "slice.h"
#include <sstream>


namespace rsutils {
namespace string {


// Pair of slices, from one or two different sources.
// These can be put together using constructs like ellipsis...
//
struct twoslice
{
    typedef slice::size_type size_type;

    slice first, second;

    twoslice( slice const & f, slice const & s )
        : first( f )
        , second( s )
    {
    }
    twoslice() {}

    size_type length() const { return first.length() + second.length(); }

    // We're empty if both parts are empty
    bool empty() const { return ! length(); }

    // By changing which part is empty, meaning is conveyed: we're invalid only if left is empty.
    // I.e., empty implies invalid but invalid does not imply empty!
    bool is_valid() const { return !! first; }
    operator bool() const { return is_valid(); }
};


// Output to ostream two strings separated by " ... ". E.g.:
//      This is part ... of a string.
// With:
//      os << ellipsis( part1, part2 );
// 
// If either part is empty, no ellipsis is output. Depending on which part is empty, meaning can be
// conveyed:
//      - if left is empty, operator bool() will return false - i.e., we're invalid
//      - if right is empty, we're valid
// So, returning an empty ellipsis is like returning an invalid state.
//
struct ellipsis : twoslice
{
    static constexpr size_type const extra_length = 5;  // " ... "

    ellipsis( slice const & f, slice const & s ) : twoslice( f, s ) {}
    ellipsis() = default;

    size_type length() const
    {
        size_type l = twoslice::length();
        if( l )
            l += extra_length;
        return l;
    }

    std::string to_string() const;
};


inline std::ostream & operator<<( std::ostream & os, ellipsis const & el )
{
    if( el.first )
    {
        os << el.first;
        if( el.second )
            os << " ... " << el.second;
    }
    else if( el.second )
        os << el.second;
    return os;
}


inline std::string ellipsis::to_string() const
{
    std::ostringstream os;
    os << *this;
    return os.str();
}


}  // namespace string
}  // namespace rsutils
