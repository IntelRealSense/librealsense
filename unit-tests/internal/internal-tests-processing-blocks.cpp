// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"

#include <vector>
#include <algorithm>
#include <memory>
#include <iostream>

#include "../unit-tests-common.h"
#include "internal-tests-common.h"
#include "../unit-tests-expected.h"

#include "../src/types.h"
#include "../src/software-device.h"
#include "../src/proc/rotation-transform.h"

const int W = 16;
const int H = 8;
const int D_BPP = 2;
const int IR_BPP = 1;
const int C_BPP = 1;

std::vector<uint8_t> create_rotation_frame_pixels(int w, int h, int bpp,
    uint8_t top_left, uint8_t top_right, uint8_t bottom_left, uint8_t bottom_right)
{
    std::vector<uint8_t> pixels(w * h * bpp, 0);

    pixels[0 * bpp] = top_left;
    pixels[(w - 1) * bpp] = top_right;
    pixels[(h - 1) * w * bpp] = bottom_left;
    pixels[(h * w - 1) * bpp] = bottom_right;

    return pixels;
}

TEST_CASE("Processing Blocks - Rotation Sanity", "[processing][software-device]")
{
    // Expect the following transformation
    //
    // --------------------        --------------
    // |4                2|        |1          2|
    // |                  |  <==   |            |
    // |                  |        |            |
    // |3                1|        |            |
    // --------------------        |            |
    //                             |            |
    //                             |3          4|
    //                             --------------

    rs2::context ctx;
    REQUIRE(make_context(SECTION_FROM_TEST_NAME, &ctx));

    // Create a synthetics frame, which holds four known points at the four edges of the frame.
    std::shared_ptr<software_device> dev = std::make_shared<software_device>();
    auto&& s = dev->add_software_sensor("depth_sensor");

    rs2_intrinsics intrinsics{ W, H, 0, 0, 0, 0, RS2_DISTORTION_NONE ,{ 0,0,0,0,0 } };

    auto depth_stream = s.add_video_stream({ RS2_STREAM_DEPTH, 0, 0, W, H, 30, D_BPP, RS2_FORMAT_Z16, intrinsics });
    auto ir_stream = s.add_video_stream({ RS2_STREAM_INFRARED, 1, 1, W, H, 30, IR_BPP, RS2_FORMAT_Y8, intrinsics });

    auto&& d_pixels = create_rotation_frame_pixels(H, W, D_BPP, 1, 2, 3, 4);
    auto&& ir_pixels = create_rotation_frame_pixels(H, W, IR_BPP, 1, 2, 3, 4);

    rotation_transform d_rt(RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME);
    rotation_transform ir_rt(RS2_FORMAT_Y8, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME);

    auto check_rotated_frame = [](frame_holder f, int BPP) {
        auto&& expected_rotated_pixels = create_rotation_frame_pixels(W, H, BPP, 4, 2, 3, 1);
        auto frame_data = f->get_frame_data();
        std::vector<uint8_t> frame_pixels(frame_data, frame_data + f->get_frame_data_size());

        bool rotated_successfully = expected_rotated_pixels == frame_pixels;
        REQUIRE(rotated_successfully);
    };

    // Pass the frame through the rotation processing block
    auto ir_callback = [=](frame_holder f)
    {
        check_rotated_frame(f.frame, IR_BPP);
    };

    auto d_callback = [=](frame_holder f)
    {
        check_rotated_frame(f.frame, D_BPP);
    };

    ir_rt.set_output_callback(frame_callback_ptr(new internal_frame_callback<decltype(ir_callback)>(ir_callback)));
    d_rt.set_output_callback(frame_callback_ptr(new internal_frame_callback<decltype(d_callback)>(d_callback)));

    // Configure sensor callback with the processing blocks' invocation
    auto s_cb = [&d_rt, &ir_rt](frame_holder f) {
        f->acquire();
        if (f->get_stream()->get_format() == RS2_FORMAT_Z16)
            d_rt.invoke(f.frame);
        else
            ir_rt.invoke(f.frame);
    };
    auto sensor_cb = frame_callback_ptr(new internal_frame_callback<decltype(s_cb)>(s_cb));

    s.open({ depth_stream, ir_stream });
    s.start(sensor_cb);
    
    rs2_stream_profile dsp;
    rs2_stream_profile irsp;
    dsp.profile = depth_stream.get();
    irsp.profile = ir_stream.get();

    // Invoke synthetic frames
    s.on_video_frame({ d_pixels.data(), [](void*) {}, 0,0,0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 1, &dsp });
    s.on_video_frame({ ir_pixels.data(), [](void*) {}, 0,0,0, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, 2, &irsp });

    s.stop();
    s.close();
}

// TODO - Add confidence rotation test
