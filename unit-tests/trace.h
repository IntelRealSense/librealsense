// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/string/from.h>


// Define our own logging macro for debugging to stdout
// Can possibly turn it on automatically based on the Catch options supplied
// on the command-line, with a custom main():
//     Catch::Session catch_session;
//     int main (int argc, char * const argv[]) {
//         return catch_session.run( argc, argv );
//     }
//     #define TRACE(X) if( catch_session.configData().verbosity == ... ) {}
// With Catch2, we can turn this into SCOPED_INFO.
#define TRACE(X) std::cout << ( rsutils::string::from() << X ).str() << std::endl
