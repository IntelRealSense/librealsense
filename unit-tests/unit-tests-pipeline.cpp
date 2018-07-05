// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <cmath>
#include "unit-tests-common.h"
#include "../include/librealsense2/rs_advanced_mode.hpp"
#include <librealsense2/hpp/rs_frame.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>

using namespace rs2;

TEST_CASE("enable default streams", "[pipeline]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    pipeline pipe(ctx);
    REQUIRE_NOTHROW(pipe.start());
    pipe.stop();
}

TEST_CASE("enable all streams", "[pipeline]")
{
	// Require at least one device to be plugged in
	rs2::context ctx;
	pipeline pipe(ctx);
	rs2::config cfg;

	cfg.enable_all_streams();
    REQUIRE_NOTHROW(pipe.start(cfg));
    pipe.stop();
}

TEST_CASE("disable stream", "[pipeline]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    pipeline pipe(ctx);
    rs2::config cfg;

    cfg.enable_stream(rs2_stream::RS2_STREAM_COLOR, 1000);
    cfg.disable_stream(rs2_stream::RS2_STREAM_COLOR, 1000);
    REQUIRE_NOTHROW(pipe.start(cfg));
    pipe.stop();
}

TEST_CASE("enable bad configuration", "[pipeline]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    pipeline pipe(ctx);
    rs2::config cfg;

    cfg.enable_stream(rs2_stream::RS2_STREAM_COLOR, 1000);
    REQUIRE_THROWS(pipe.start(cfg));
}