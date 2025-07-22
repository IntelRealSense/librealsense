// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <cstdint>
#include <iosfwd>


namespace rsutils {
namespace type {


class fourcc
{
public:
    using value_type = uint32_t;

private:
    value_type _4cc;

public:
    template< typename T >
    fourcc( const T a, const T b, const T c, const T d )
        : _4cc( ( static_cast< uint32_t >( a ) << 24 ) | ( static_cast< uint32_t >( b ) << 16 )
                | ( static_cast< uint32_t >( c ) << 8 ) | ( static_cast< uint32_t >( d ) << 0 ) )
    {
    }
    explicit fourcc( value_type fcc )
        : _4cc( fcc )
    {
    }
    fourcc( fourcc const & other ) = default;

    value_type get() const noexcept { return _4cc; }
    operator value_type() const noexcept { return get(); }

    bool operator==( fourcc const & other ) const noexcept { return get() == other.get(); }
    bool operator!=( fourcc const & other ) const noexcept { return ! operator==( other ); }
};


std::ostream & operator<<( std::ostream &, fourcc const & );


}  // namespace type
}  // namespace rsutils
