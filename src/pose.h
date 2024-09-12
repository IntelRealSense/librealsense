// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include "basics.h"
#include "float3.h"
#include <librealsense2/h/rs_sensor.h>
#include <ostream>
#include <cstring>  // memcmp


namespace librealsense {


#pragma pack( push, 1 )
struct pose
{
    float3x3 orientation;
    float3 position;
};
#pragma pack( pop )

inline bool operator==( const pose & a, const pose & b ) { return a.orientation == b.orientation && a.position == b.position; }
inline float3 operator*( const pose & a, const float3 & b ) { return a.orientation * b + a.position; }
inline pose operator*( const pose & a, const pose & b ) { return{ a.orientation * b.orientation, a * b.position }; }

inline pose inverse( const pose & a )
{
    auto inv = transpose( a.orientation );
    return { inv, inv * a.position * -1 };
}

inline pose to_pose( const rs2_extrinsics & a )
{
    pose r{};
    for( int i = 0; i < 3; i++ )
        r.position[i] = a.translation[i];
    for( int j = 0; j < 3; j++ )
        for( int i = 0; i < 3; i++ )
            r.orientation( i, j ) = a.rotation[j * 3 + i];
    return r;
}

inline rs2_extrinsics from_pose( pose a )
{
    rs2_extrinsics r;
    for( int i = 0; i < 3; i++ )
        r.translation[i] = a.position[i];
    for( int j = 0; j < 3; j++ )
        for( int i = 0; i < 3; i++ )
            r.rotation[j * 3 + i] = a.orientation( i, j );
    return r;
}

inline std::ostream & operator<<( std::ostream & stream, const pose & elem )
{
    return stream << "Position:\n " << elem.position << "\n Orientation :\n" << elem.orientation;
}

inline rs2_extrinsics identity_matrix()
{
    rs2_extrinsics r;
    // Do it the silly way to avoid infite warnings about the dangers of memset
    for( int i = 0; i < 3; i++ )
        r.translation[i] = 0.f;
    for( int j = 0; j < 3; j++ )
        for( int i = 0; i < 3; i++ )
            r.rotation[j * 3 + i] = ( i == j ) ? 1.f : 0.f;
    return r;
}

inline rs2_extrinsics inverse( const rs2_extrinsics & a )
{
    auto p = to_pose( a );
    return from_pose( inverse( p ) );
}

// The extrinsics on the camera ("raw extrinsics") are in milimeters, but LRS works in meters
// Additionally, LRS internal algorithms are
// written with a transposed matrix in mind! (see rs2_transform_point_to_point)
rs2_extrinsics to_raw_extrinsics( rs2_extrinsics );
rs2_extrinsics from_raw_extrinsics( rs2_extrinsics );


}  // namespace librealsense

std::ostream & operator<<( std::ostream &, rs2_extrinsics const & );
std::ostream & operator<<( std::ostream &, rs2_intrinsics const & );

LRS_EXTENSION_API bool operator==( const rs2_extrinsics & a, const rs2_extrinsics & b );
inline bool operator==( const rs2_intrinsics & a, const rs2_intrinsics & b )
{
    return std::memcmp( &a, &b, sizeof( a ) ) == 0;
}
