// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_sensor.hpp>
#include "../../common/tiny-profiler.h"
#include "./../unit-tests-common.h"
#include "./../src/environment.h"

using namespace librealsense;
using namespace librealsense::platform;

// Check resolver
// Check processing blocks factories
// check start callbacks
// check init stream profiles
// check sensor d'tor - stops raw sensor, closes stream..
// check sensor deallocation - resources
