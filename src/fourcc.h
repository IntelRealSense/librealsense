// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <type_traits>
#include <cstdint>


namespace librealsense {


template< typename T >
uint32_t rs_fourcc( const T a, const T b, const T c, const T d )
{
    static_assert( ( std::is_integral< T >::value ), "rs_fourcc supports integral built-in types only" );
    return   ( ( static_cast< uint32_t >( a ) << 24 )
             | ( static_cast< uint32_t >( b ) << 16 )
             | ( static_cast< uint32_t >( c ) << 8 )
             | ( static_cast< uint32_t >( d ) << 0 ) );
}


}  // namespace librealsense
