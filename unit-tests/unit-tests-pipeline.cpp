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

# define SECTION_FROM_TEST_NAME space_to_underscore(Catch::getCurrentContext().getResultCapture()->getCurrentTestName()).c_str()


TEST_CASE("enable default streams", "[pipeline]")
{
    rs2::context ctx;
    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    pipeline pipe;
    REQUIRE_NOTHROW(pipe.start());
    pipe.stop();
}

TEST_CASE("enable all streams", "[pipeline]")
{
    rs2::context ctx;
    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    pipeline pipe;
    rs2::config cfg;

    cfg.enable_all_streams();
    REQUIRE_NOTHROW(pipe.start(cfg));
    pipe.stop();
}

TEST_CASE("disable stream", "[pipeline]")
{
    rs2::context ctx;
    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(rs2_stream::RS2_STREAM_COLOR, 1000);
    cfg.disable_stream(rs2_stream::RS2_STREAM_COLOR, 1000);
    REQUIRE_NOTHROW(pipe.start(cfg));
    pipe.stop();
}

TEST_CASE("enable bad configuration", "[pipeline]")
{
    rs2::context ctx;
    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(rs2_stream::RS2_STREAM_COLOR, 1000);
    REQUIRE_THROWS(pipe.start(cfg));
}

TEST_CASE("default playback config", "[pipeline]")
{
    rs2::context ctx;
    if (!make_context(SECTION_FROM_TEST_NAME, &ctx))
        return;

    std::string file_name = "C:\\tmp\\enable_all_streams.bag";
    std::vector<stream_profile> rec_streams;
    std::vector<stream_profile> pb_streams;

    {
        pipeline pipe;
        rs2::config cfg;

        cfg.enable_all_streams();
        cfg.enable_record_to_file(file_name);
        auto profile = pipe.start(cfg);
        int frames = 10;
        while (frames--)
            pipe.wait_for_frames();
        pipe.stop();
        rec_streams = profile.get_streams();
    }

    {
        pipeline pipe;
        rs2::config cfg;
        cfg.enable_device_from_file(file_name);
        auto profile = pipe.start(cfg);
        pipe.wait_for_frames();
        pipe.stop();
        pb_streams = profile.get_streams();
    }

    REQUIRE(rec_streams == pb_streams);
}