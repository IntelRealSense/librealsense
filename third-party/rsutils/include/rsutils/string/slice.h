// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <ostream>
#include <cassert>
#include <cstring>  // std::strlen


namespace rsutils {
namespace string {


// Same as std::string, except not null-terminated, and does not manage its own memory! Similar to the C++17
// std::string_view, except it can be expanded to be non-const, too.
//
// This is meant to point into an existing string and have a short life-time. If the underlying memory is removed, e.g.:
//      slice foo()
//      {
//          std::string bar = "haha";
//          return slice(bar);
//      }
// Not good!
//
// This can come in very useful when wanting to break a string into parts but without incurring allocations or copying,
// or changing of the original string contents (like strtok does), or when the original memory is not null-terminated.
//
class slice
{
public:
    typedef char const * const_iterator;
    typedef std::string::size_type size_type;
    typedef std::string::value_type value_type;

private:
    const_iterator _begin, _end;

public:
    slice( const_iterator begin, const_iterator end )
        : _begin( begin )
        , _end( end )
    {
        assert( begin <= end );
    }
    slice( char const * str, size_type length )
        : slice( str, str + length )
    {
    }
    explicit slice( char const * str )
        : slice( str, std::strlen( str ) )
    {
    }
    slice( std::string const & str )
        : slice( str.data(), str.length() )
    {
    }
    slice()
        : _begin( nullptr )
        , _end( nullptr )
    {
    }

    bool empty() const { return begin() == end(); }
    size_type length() const { return end() - begin(); }
    operator bool() const { return ! empty(); }

    void clear() { _end = _begin; }

    const_iterator begin() const { return _begin; }
    const_iterator end() const { return _end; }

    value_type front() const { return *begin(); }
    value_type back() const { return end()[-1]; }
};


inline std::ostream & operator<<( std::ostream & os, slice const & str )
{
    return os.write( str.begin(), str.length() );
}


inline bool operator==( std::string const & left, slice const & right )
{
    return 0 == left.compare( 0, std::string::npos, right.begin(), right.length() );
}
inline bool operator!=( std::string const & left, slice const & right )
{
    return 0 != left.compare( 0, std::string::npos, right.begin(), right.length() );
}


}  // namespace string
}  // namespace rsutils
