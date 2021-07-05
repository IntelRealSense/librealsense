// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <easylogging++.h>

#include "../catch.h"

#include "../approx.h"

//#cmake:add-file ../../src/types.h
#include <src/types.h>

using namespace librealsense;

#include "rot.h"


TEST_CASE( "pose vs extrinsics", "[types]" )
{
    pose const POSE = { { {0,1,2}, {3,4,5}, {6,7,8} }, { 0,0,1 } };
    rs2_extrinsics const EXTR = { { 0,1,2,3,4,5,6,7,8 }, {0,0,1} };

    // Pose memory layout same exact as extrinsics:
    CHECK( 0 == memcmp( &POSE, &EXTR, 9 * sizeof( float ) ) );
    pose p = to_pose( EXTR );
    CHECK( 0 == memcmp( &p, &EXTR, 9 * sizeof( float ) ) );

    // To/from pose do nothing, really!
    CHECK( to_pose( EXTR ) == POSE );
    CHECK( from_pose( POSE ) == EXTR );
    CHECK( to_pose( from_pose( POSE )) == POSE );
}

TEST_CASE( "float3x3 operator()", "[types]" )
{
    float3x3 m = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    // column-major, meaning first number is the column!
    INFO( m );
    INFO( "(0,0) " << m( 0, 0 ) << " (0,1) " << m( 0, 1 ) << " (0,2) " << m( 0, 2 ) );
    INFO( "(1,0) " << m( 1, 0 ) << " (1,1) " << m( 1, 1 ) << " (1,2) " << m( 1, 2 ) );
    INFO( "(2,0) " << m( 2, 0 ) << " (2,1) " << m( 2, 1 ) << " (2,2) " << m( 2, 2 ) );

    CHECK( m( 1, 0 ) == 2 );
}


// These check for approximate equality for the different types
inline bool eq( float3 const & a, float3 const & b )
{
    return a.x == approx( b.x ) && a.y == approx( b.y ) && a.z == approx( b.z );
}
inline bool eq( float3x3 const & a, float3x3 const & b )
{
    return eq( a.x, b.x ) && eq( a.y, b.y ) && eq( a.z, b.z );
}
inline bool eq( pose const & a, pose const & b )
{
    return eq( a.orientation, b.orientation ) && eq( a.position, b.position );
}
inline bool eq( rs2_extrinsics const & a, rs2_extrinsics const & b )
{
    return eq( *(pose const *)&a, *(pose const *)&b );
}

float dot( float3 const & a, float3 const & b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
float3 mult( const float3x3 & a, const float3 & b )
{
    // This is the version in the "World's tiniest linear algebra library":
    //     return a.x*b.x + a.y*b.y + a.z*b.z;
    // And this is the conventional way:
    return { dot( a.x,b ), dot( a.y,b ), dot( a.z,b ) };
}

TEST_CASE( "m*v", "[types]" )
{
    float3x3 m = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    float3   v = { 1, -2, 3 };

    INFO( m );
    INFO( v );
 
    // This fails (but shouldn't) with our matrix math:
    //FAILS:CHECK( m * v == mult( m, v ) );

    // Our math is all transposed! (e.g., see rs2_transform_point_to_point)
    // So intead we have:
    CHECK( transpose( m ) * v == mult( m, v ) );
}

TEST_CASE( "m*v (rotation matrix)", "[types]" )
{
    float3x3 m = rotx( 30 );
    float3   v = { 1, -2, 3 };

    INFO( m );
    INFO( "(0,0) " << m( 0, 0 ) << " (0,1) " << m( 0, 1 ) << " (0,2) " << m( 0, 2 ) );
    INFO( "(1,0) " << m( 1, 0 ) << " (1,1) " << m( 1, 1 ) << " (1,2) " << m( 1, 2 ) );
    INFO( "(2,0) " << m( 2, 0 ) << " (2,1) " << m( 2, 1 ) << " (2,2) " << m( 2, 2 ) );
 
    //FAILS:CHECK( m * v == mult( m, v ) );
    //FAILS:CHECK_FALSE( transpose( m ) * v == mult( m, v ) );
}

float determinant( float3x3 const & r )
{
    // |A| = aei + bfg + cdh - ceg - bdi - afh
    return r.x.x * r.y.y * r.z.z  // aei
        + r.x.y * r.y.z * r.z.x   // bfg
        + r.x.z * r.y.x * r.z.y   // cdh
        - r.x.z * r.y.y * r.z.x   // ceg
        - r.x.y * r.y.x * r.z.z   // bdi
        - r.x.x * r.y.z * r.z.y;  // afh
}

TEST_CASE( "rotation matrix", "[types]" )
{
    float3x3 I = { {1,0,0}, {0,1,0}, {0,0,1} };
    float3x3 rot = rotx( 30 );

    // A valid rotation matrix must satisfy:
    //     transpose(r)*r == identity
    //     determinant(r) == 1
    INFO( "\nrot=\n" << rot );
    INFO( "\nrot_T=\n" << transpose( rot ) );
    CHECK( transpose( rot ) * rot == I );
    CHECK( determinant( rot ) == 1 );
}

/*
[ux vx wx tx] -1   ( [1 0 0 tx]   [ux vx wx 0] ) -1
[uy vy wy ty]      ( [0 1 0 ty]   [uy vy wy 0] )
[uz vz wz tz]    = ( [0 0 1 tz] * [uz vz wz 0] )
[ 0  0  0  1]      ( [0 0 0  1]   [ 0  0  0 1] )

To place an object at a given position and orientation, you first rotate the object,
then second translate it to its new position. This corresponds to placing the rotation
matrix on the right and the translation matrix on the left.

                   [ux vx wx 0] -1   [1 0 0 tx] -1
                   [uy vy wy 0]      [0 1 0 ty]
                 = [ux vz wz 0]    * [0 0 1 tz]
                   [ 0  0  0 1]      [0 0 0  1]

The inverse of a matrix product is the product of the inverse matrices ordered in reverse.

                   [ux uy uz 0]   [1 0 0 -tx]
                   [vx vy vz 0]   [0 1 0 -ty]
                 = [wx wy wz 0] * [0 0 1 -tz]
                   [ 0  0  0 1]   [0 0 0  1 ]

The inverse of a rotation matrix is the rotation matrix's transpose.
The inverse of a translation matrix is the translation matrix with the opposite signs
on each of the translation components.

                   [ux uy uz -ux*tx-uy*ty-uz*tz]
                   [vx vy vz -vx*tx-vy*ty-vz*tz]
                 = [wx wy wz -wx*tx-wy*ty-wz*tz]
                   [ 0  0  0          1        ]

                   [ux uy uz -dot(u,t)]
                   [vx vy vz -dot(v,t)]
                 = [wx wy wz -dot(w,t)]
                   [ 0  0  0     1    ]
*/

pose inv( pose const & p )
{
    // This is the version in the "World's tiniest linear algebra library":
    //     auto inv = transpose(a.orientation); return{ inv, inv * a.position * -1 };
    // But it's built on a mult that's transposed so in essence the 't' part is wrong:
    auto ot = transpose( p.orientation );
    return{ ot, mult( ot, p.position ) * -1 };
}
TEST_CASE( "inverse != inv", "[types]" )
{
    float3x3 rot = {
        { 0.999899744987488f,  0.014070882461965f, -0.001586908474565f},
        {-0.014017328619957f,  0.999457061290741f,  0.029818318784237f},
        { 0.002005616901442f, -0.029793085530400f,  0.999554097652435f}
    };
    float3 tran = { -0.000100966520607471f, 0.013899585723876953f, -0.004260723590850830f };

    auto p = pose{ rot, tran };

    INFO( "\np= " << std::setprecision( 15 ) << p );
    INFO( "\ninv(p)= " << std::setprecision( 15 ) << inv( p ) );
    INFO( "\ninverse(p)= " << std::setprecision( 15 ) << inverse( p ) );
    //FAILS:CHECK( eq( inverse( p ), inv( p ) ) );
    CHECK_FALSE( eq( inverse( p ), inv( p ) ) );
}

pose inv2( pose const & p )
{
    // This version negates the position before multiplication; should be the same
    auto ot = transpose( p.orientation );
    return{ ot, mult( ot, p.position * -1 ) };
}
TEST_CASE( "inv == inv2", "[types]" )
{
    float3x3 rot = {
        { 0.999899744987488f,  0.014070882461965f, -0.001586908474565f},
        {-0.014017328619957f,  0.999457061290741f,  0.029818318784237f},
        { 0.002005616901442f, -0.029793085530400f,  0.999554097652435f}
    };
    float3 tran = { -0.000100966520607471f, 0.013899585723876953f, -0.004260723590850830f };
    auto p = pose{ rot, tran };

    INFO( "\np= " << std::setprecision( 15 ) << p );
    INFO( "\ninv(p)= " << std::setprecision( 15 ) << inv( p ) );
    INFO( "\ninv2(p)= " << std::setprecision( 15 ) << inv2( p ) );
    CHECK( inv2( p ) == inv( p ) );
    CHECK( eq( inv2( inv2( p ) ), p ) );
}

TEST_CASE( "inverse of inverse (not a rot mat)", "[types]" )
{
    // the rotation matrix here is not valid (has no inverse)
    rs2_extrinsics extr{ { 0,1,2,3,4,5,6,7,8 }, {1,2,3} };
    pose p = to_pose( extr );
    CHECK_FALSE( inverse( inverse( p ) ) == p );
}

TEST_CASE( "inverse of inverse (rot mat)", "[types]" )
{
    float3x3 rot = rotx( 30 );
    float3 tran = { -0.100967f, 13.899586f, -4.260724f };

    pose p = { rot, tran };

    INFO( "\np=\n" << std::setprecision( 15 ) << p );
    INFO( "\ninv(p)=\n" << std::setprecision( 15 ) << inv( p ) );
    INFO( "\ninv(inv(p))=\n" << inv( inv( p ) ) );
    INFO( "\ninverse(inverse(p))=\n" << inverse( inverse( p ) ) );
    CHECK( inverse( inverse( p ) ) == p );
}

TEST_CASE( "inverse of inverse (extr)", "[types]" )
{
    float3x3 rot = {
        { 0.999899744987488f,  0.014070882461965f, -0.001586908474565f},
        {-0.014017328619957f,  0.999457061290741f,  0.029818318784237f},
        { 0.002005616901442f, -0.029793085530400f,  0.999554097652435f}
    };
    float3 tran = { -0.000100966520607471f, 0.013899585723876953f, -0.004260723590850830f };

    rs2_extrinsics extr = from_pose( { rot, tran } );

    CHECK( eq( inverse( inverse( extr ) ), extr ) );
}
