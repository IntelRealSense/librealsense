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
#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"

// Define our own logging macro for debugging to stdout
// Can possibly turn it on automatically based on the Catch options supplied
// on the command-line, with a custom main():
//     Catch::Session catch_session;
//     int main (int argc, char * const argv[]) {
//         return catch_session.run( argc, argv );
//     }
//     #define TRACE(X) if( catch_session.configData().verbosity == ... ) {}
// With Catch2, we can turn this into SCOPED_INFO.
#define TRACE(X) do { \
    std::cout << X << std::endl; \
    } while(0)



