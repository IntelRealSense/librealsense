// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

// #include <easylogging++.h>

#include <iostream>

#define DBG std::cout // CLOG(DEBUG, "librealsense")
#define ERR std::cout // CLOG(ERROR, "librealsense")
#define WRN std::cout // CLOG(WARNING, "librealsense")
#define INF std::cout // CLOG(INFO, "librealsense")