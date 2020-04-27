// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

// Catch also defines CHECK(), and so we have to undefine it or we get compilation errors!
#undef CHECK

// Let Catch define its own main() function
#define CATCH_CONFIG_MAIN
#ifdef _MSC_VER
/*
The .hpp gives the following warning C4244:
C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.16.27023\include\algorithm(2583): warning C4244: 'argument': conversion from '_Diff' to 'int', possible loss of data
C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Tools\MSVC\14.16.27023\include\algorithm(2607): note: see reference to function template instantiation 'void std::_Random_shuffle1<_RanIt,Catch::TestRegistry::RandomNumberGenerator>(_RanIt,_RanIt,_RngFn &)' being compiled
        with
        [
            _RanIt=std::_Vector_iterator<std::_Vector_val<std::_Simple_types<Catch::TestCase>>>,
            _RngFn=Catch::TestRegistry::RandomNumberGenerator
        ]
c:\work\git\lrs\unit-tests\algo\../catch/catch.hpp(5788): note: see reference to function template instantiation 'void std::random_shuffle<std::_Vector_iterator<std::_Vector_val<std::_Simple_types<_Ty>>>,Catch::TestRegistry::RandomNumberGenerator&>(_RanIt,_RanIt,_RngFn)' being compiled
        with
        [
            _Ty=Catch::TestCase,
            _RanIt=std::_Vector_iterator<std::_Vector_val<std::_Simple_types<Catch::TestCase>>>,
            _RngFn=Catch::TestRegistry::RandomNumberGenerator &
        ]
*/
#pragma warning (push)
#pragma warning (disable : 4244)    // 'this': used in base member initializer list
#endif
#include "../catch/catch.hpp"
#ifdef _MSC_VER
#pragma warning (pop)
#endif

// We need to compare floating point values, therefore we need an approximation
// function, which Catch provides for us:
//     REQUIRE( performComputation() == Approx( 2.1 ));
// (see https://github.com/catchorg/Catch2/blob/master/docs/assertions.md)
// For example (with the default epsilon):
//     2.61007666588 ~= 2.61007662723
// This may not be good enough for us and want to control the epsilon. By default
// Approx sets its epsilon to:
//     std::numeric_limits<float>::epsilon()*100
// And to set a custom epsilon:
//     REQUIRE( <val> == Approx(2.1).epsilon(0.01) );  // allow up to 1% diff
// So we want a macro that does this automatically:
#define __ALGO_EPSILON (std::numeric_limits<float>::epsilon()*1)
#define approx(X) Approx(X).epsilon(__ALGO_EPSILON)
// Because we have our own macro (and because it's more verbose) we do NOT want
// to use the literal that Catch supplies:
//     using namespace Catch::literals;
//     REQUIRE( performComputation() == 2.1_a );

//#cmake:add-file ../../src/types.h
#include <types.h>

using namespace librealsense;

#include "rot.h"


TEST_CASE( "rotx", "[types]" )
{
}


TEST_CASE( "to pose", "[types]" )
{
    pose const EXPECTED = { { {0,1,2}, {3,4,5}, {6,7,8} }, { 0,0,1 } };
    CHECK( to_pose( rs2_extrinsics{ { 0,1,2,3,4,5,6,7,8 }, {0,0,1} } ) == EXPECTED );
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

#if 0
float dot( float3 const & a, float3 const & b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
#endif
float3 mult( const float3x3 & a, const float3 & b )
{
    //return a.x*b.x + a.y*b.y + a.z*b.z;
    return { dot( a.x,b ), dot( a.y,b ), dot( a.z,b ) };
}
pose inv( pose const & p )
{
    auto ot = transpose( p.orientation );
    //return{ inv, inv * a.position * -1 };
    return{ ot, mult( ot, p.position ) * -1 };
}
pose inv2( pose const & p )
{
    auto ot = transpose( p.orientation );
    //return{ inv, inv * a.position * -1 };
    return{ ot, mult( ot, p.position * -1 ) };
}

TEST_CASE( "m*v", "[types]" )
{
    float3x3 m = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    float3   v = { 1, -2, 3 };

    INFO( m );
    INFO( "(0,0) " << m(0,0) << " (0,1) " << m(0,1) << " (0,2) " << m(0,2) );
    INFO( "(1,0) " << m(1,0) << " (1,1) " << m(1,1) << " (1,2) " << m(1,2) );
    INFO( "(2,0) " << m(2,0) << " (2,1) " << m(2,1) << " (2,2) " << m(2,2) );
    
    CHECK( m * v == mult( m, v ) );
    CHECK_FALSE( transpose( m ) * v == mult( m, v ) );
}

TEST_CASE( "m*v (rot)", "[types]" )
{
    float3x3 m = rotx( 30 );
    float3   v = { 1, -2, 3 };

    INFO( m );
    INFO( "(0,0) " << m( 0, 0 ) << " (0,1) " << m( 0, 1 ) << " (0,2) " << m( 0, 2 ) );
    INFO( "(1,0) " << m( 1, 0 ) << " (1,1) " << m( 1, 1 ) << " (1,2) " << m( 1, 2 ) );
    INFO( "(2,0) " << m( 2, 0 ) << " (2,1) " << m( 2, 1 ) << " (2,2) " << m( 2, 2 ) );
 
    CHECK( m * v == mult( m, v ) );
    CHECK_FALSE( transpose( m ) * v == mult( m, v ) );
}

TEST_CASE( "to_pose", "[types]" )
{
    rs2_extrinsics extr = {
        { 1, 2, 3, 4, 5, 6, 7, 8, 9 },
        { 1, -2, 3 }
    };
    pose p = to_pose( extr );
    INFO( p );
    CHECK( p.orientation.x.y == 2 );
    CHECK( from_pose( p ) == extr );
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

TEST_CASE( "inverse of inverse (rot mat)", "[types]" )
{
    float3x3 I = { {1,0,0}, {0,1,0}, {0,0,1} };
    float3x3 rot = rotx( 30 );

    // A valid rotation matrix must satisfy:
    //     transpose(r)*r == identity
    //     determinant(r) == 1
    INFO( "\nrot=\n" << rot );
    INFO( "\nrot_T=\n" << transpose(rot) );
    CHECK( transpose( rot ) * rot == I );
    CHECK( determinant( rot ) == 1 );

    float3 tran = { -0.100967f, 13.899586f, -4.260724f };

    rs2_extrinsics extr = from_pose( { rot, tran } );
    auto p = to_pose( extr );

    INFO( "extr= " << extr );
    INFO( "inverse(inverse(extr))= " << inverse( inverse( extr ) ) );
    INFO( "inv(inv(p))= " << inv( inv( p ) ) );
    INFO( "inv(p)*p= " << inv( p ) * p );
    
    CHECK( inverse( inverse( extr ) ) == extr );
}

inline bool eq( float3 const & a, float3 const & b )
{
    return a.x == approx( b.x ) && a.y == approx( b.y ) && a.z == approx( b.z );
}

inline bool eq( float3x3 const & a, float3x3 const & b )
{
    return eq( a.x, b.x )  &&  eq( a.y, b.y )  &&  eq( a.z, b.z );
}

inline bool eq( pose const & a, pose const & b )
{
    return eq( a.orientation, b.orientation )  &&  eq( a.position, b.position );
}

TEST_CASE( "inverse of inverse (extr)", "[types]" )
{
    float3x3 I = { {1,0,0}, {0,1,0}, {0,0,1} };
    float3x3 rot = {
        { 0.999899744987488f,  0.014070882461965f, -0.001586908474565f},
        {-0.014017328619957f,  0.999457061290741f,  0.029818318784237f},
        { 0.002005616901442f, -0.029793085530400f,  0.999554097652435f}
    };

    // A valid rotation matrix must satisfy:
    //     transpose(r)*r == identity
    //     determinant(r) == 1
    CHECK( eq( transpose( rot ) * rot, I ));
    CHECK( determinant( rot ) == approx(1.f) );

    float3 tran = { -0.000100966520607471f, 0.013899585723876953f, -0.004260723590850830f };

    auto p = pose{ rot, tran };

    INFO( "\np= " << std::setprecision( 15 ) << p );
    INFO( "\ninv(p)= " << std::setprecision( 15 ) << inv( p ) );
    INFO( "\ninverse(p)= " << std::setprecision(15) << inverse(p) );
    CHECK( eq( inverse( p ), inv( p ) ) );

    INFO( "\ninverse(inverse(p)= " << std::setprecision( 15 ) << inverse( inverse( p ) ) );
    CHECK( eq( inverse( inverse( p ) ), p ) );
    INFO( "\ninv(inv(p)= " << std::setprecision( 15 ) << inv( inv( p ) ) );
    CHECK( eq( inv( inv( p ) ), p ) );
    INFO( "\np= " << std::setprecision( 15 ) << p );
    INFO( "\ninv2(p)= " << std::setprecision( 15 ) << inv2( p ) );
    CHECK( inv2( p ) == inv( p ) );
    CHECK( eq( inv2( inv2( p )), p ));
}

TEST_CASE( "inverse of inverse (not a rot mat)", "[types]" )
{
    // the rotation matrix here is not valid (has no inverse)
    rs2_extrinsics extr{ { 0,1,2,3,4,5,6,7,8 }, {1,2,3} };
    CHECK_FALSE( inverse( inverse( extr ) ) == extr );
}
