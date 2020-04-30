// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once


// Catch defines CHECK() and so does EL++, and so we have to undefine it or we get compilation errors!
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
