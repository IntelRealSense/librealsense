// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#ifdef BUILD_EASYLOGGINGPP
#include "easylogging++.h"
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif
#endif