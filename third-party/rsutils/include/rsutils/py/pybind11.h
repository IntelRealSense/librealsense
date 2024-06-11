// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#pragma warning(push)
// third-party\pybind11\include\pybind11\pybind11.h(232): warning C4702: unreachable code
#if defined(_MSC_VER) && _MSC_VER < 1910  // VS 2015's MSVC
#  pragma warning(disable: 4702)
#endif
#include <pybind11/pybind11.h>
#pragma warning(pop)

// convenience functions
#include <pybind11/operators.h>

// STL conversions
#include <pybind11/stl.h>

// std::chrono::*
#include <pybind11/chrono.h>

// makes certain STL containers opaque to prevent expensive copies
#include <pybind11/stl_bind.h>

// makes std::function conversions work
#include <pybind11/functional.h>

// and enable a bridge to/from rsutils::json
#include "json.h"


namespace py = pybind11;
using namespace pybind11::literals;


// "When calling a C++ function from Python, the GIL is always held"
// -- since we're not being called from Python but instead are calling it,
// we need to acquire it to not have issues with other threads...
#define FN_FWD_CALL( CLS, FN_NAME, CODE )                                                                              \
    try                                                                                                                \
    {                                                                                                                  \
        py::gil_scoped_acquire gil;                                                                                    \
        CODE                                                                                                           \
    }                                                                                                                  \
    catch( std::exception const & e )                                                                                  \
    {                                                                                                                  \
        LOG_ERROR( "EXCEPTION in python " #CLS "." #FN_NAME ": " << e.what() ); \
    }                                                                                                                  \
    catch( ... )                                                                                                       \
    {                                                                                                                  \
        LOG_ERROR( "UNKNOWN EXCEPTION in python " #CLS "." #FN_NAME ); \
    }
#define FN_FWD( CLS, FN_NAME, PY_ARGS, FN_ARGS, CODE )                                                                 \
    #FN_NAME, []( CLS & self, std::function < void PY_ARGS > callback ) {                                              \
        self.FN_NAME( [&self,callback] FN_ARGS { FN_FWD_CALL( CLS, FN_NAME, CODE ) } );                                \
    }
#define FN_FWD_R( CLS, FN_NAME, RV, PY_ARGS, FN_ARGS, CODE )                                                           \
    FN_FWD_R_( CLS, FN_NAME, decltype(RV), RV, PY_ARGS, FN_ARGS, CODE )
#define FN_FWD_R_( CLS, FN_NAME, RET, RV, PY_ARGS, FN_ARGS, CODE )                                                     \
    #FN_NAME, []( CLS & self, std::function < RET PY_ARGS > callback ) {                                               \
        self.FN_NAME( [&self, callback] FN_ARGS { FN_FWD_CALL( CLS, FN_NAME, CODE ) return RV; } );                    \
    }
