// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifdef BUILD_EASYLOGGINGPP
#ifndef NO_ELPP_INIT
#include <third-party/easyloggingpp/src/easylogging++.h>

// ELPP requires static initialization, otherwise nothing will work:
//
INITIALIZE_EASYLOGGINGPP
//
// The above can be disabled with NO_ELPP_INIT! This is needed in special cases such as librealsense
// where other static initialization is used to automatically set up the logging mechanism based on
// environment variables.

#endif  // ! NO_ELPP_INIT
#endif  // BUILD_EASYLOGGINGPP
