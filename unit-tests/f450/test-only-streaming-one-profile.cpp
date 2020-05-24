// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file f450-common.h

#include "f450-common.h"


TEST_CASE( "only streaming - one profile", "[F450]" ) {
    using namespace std;

    rs2::pipeline pipe;

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_RAW16);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_RAW16);

    for (size_t i = 0; i < 3; i++)
    {
        pipe.start(cfg);

        int frameCounter = 0;
        while (frameCounter < 20)
        {
            rs2::frameset data = pipe.wait_for_frames();

            ++frameCounter;
        }

        pipe.stop();
        REQUIRE(frameCounter == 20);
    }
    
}
