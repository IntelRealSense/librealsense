// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

//#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API

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
#if ! defined( NO_CATCH_CONFIG_MAIN )
#define CATCH_CONFIG_MAIN
#endif
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

#include <limits>

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

// Define our own logging macro for debugging to stdout
// Can possibly turn it on automatically based on the Catch options supplied
// on the command-line, with a custom main():
//     Catch::Session catch_session;
//     int main (int argc, char * const argv[]) {
//         return catch_session.run( argc, argv );
//     }
//     #define TRACE(X) if( catch_session.configData().verbosity == ... ) {}
// With Catch2, we can turn this into SCOPED_INFO.
#define TRACE(X) std::cout << (std::string)( librealsense::to_string() << X ) << std::endl
