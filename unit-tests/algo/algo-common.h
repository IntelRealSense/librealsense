// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

//#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API

#include "../catch.h"

#include <easylogging++.h>

#if defined( __ALGO_EPSILON )
#define __EPSILON __ALGO_EPSILON
// #else leave default
#endif
#include "../approx.h"

#include "../trace.h"
