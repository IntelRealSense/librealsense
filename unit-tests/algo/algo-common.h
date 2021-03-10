// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

//#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API

#if ! defined( NO_CATCH_CONFIG_MAIN )
#define CATCH_CONFIG_MAIN
#endif
#include "../catch.h"

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

#if defined( __ALGO_EPSILON )
#define __EPSILON __ALGO_EPSILON
// #else leave default
#endif
#include "../approx.h"

#include "../trace.h"
