// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////
// This set of tests is valid only for the R200 camera //
/////////////////////////////////////////////////////////

#if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(DS5_TEST) )

#include <climits>
#include <sstream>
#include <vector>


#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-live-ds-common.h"
#include "librealsense/rs.hpp"

//////////////////////////
// Controls and Options //
//////////////////////////
TEST_CASE("RS4XX device supports all required options", "[live] [RS4XX]")
{
    rs::log_to_console(rs::log_severity::warn);
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    const std::map<std::string, std::vector<rs_option>> rs4xx_skew_options
    {
        { "Intel RealSense RS400p", { RS_OPTION_COLOR_GAIN, RS_OPTION_R200_LR_EXPOSURE, RS_OPTION_HARDWARE_LOGGER_ENABLED }},
        { "Intel RealSense RS410a", { RS_OPTION_COLOR_GAIN, RS_OPTION_R200_LR_EXPOSURE, RS_OPTION_HARDWARE_LOGGER_ENABLED,
                                      RS_OPTION_RS4XX_PROJECTOR_MODE, RS_OPTION_RS4XX_PROJECTOR_PWR }},
    };

    // For each device
    for (int i = 0; i < device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, i, require_no_error());
        REQUIRE(dev != nullptr);

        auto it = rs4xx_skew_options.find(dev->get_name());
        REQUIRE(it != rs4xx_skew_options.end());

        for (size_t i = 0; i < RS_OPTION_COUNT; ++i)
        {
            INFO("Checking support for " << (rs::option)i);
            if (std::find(it->second.begin(), it->second.end(), i) != it->second.end())
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 1);
            }
            else
            {
                REQUIRE(rs_device_supports_option(dev, (rs_option)i, require_no_error()) == 0);
            }
        }
    }
}

TEST_CASE("RS4XX XU Laser Power Mode and Power", "[live] [RS4XX]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    INFO("Device name is " << name);

    double lsr_mode_init{}, lsr_power_init{}, min{}, max{}, step{}, def{};
    rs::option opt_lsr_mode = rs::option::rs4xx_projector_mode;
    rs::option opt_lsr_pwr = rs::option::rs4xx_projector_pwr;

    if ((dev->supports_option(opt_lsr_mode)) && (dev->supports_option(opt_lsr_pwr)))
    {
        dev->get_options(&opt_lsr_mode, 1, &lsr_mode_init);
        dev->get_options(&opt_lsr_pwr, 1, &lsr_power_init);
        INFO("Initial laser mode is " << lsr_mode_init);
        INFO("Initial laser power is " << lsr_power_init);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        // Test laser mode control
        dev->get_option_range(opt_lsr_mode, min, max, step, def);

        double res = 0.;
        for (uint8_t j = 0; j < 2; j++)
        {
            INFO("Laser mode control test: iteration " << j);
            for (double set_val = min; set_val <= max; set_val += step)
            {
                dev->set_options(&opt_lsr_mode, 1, &set_val);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                dev->get_options(&opt_lsr_mode, 1, &res);
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                REQUIRE(set_val == res);
            }
        }

        // Test laser power
        // Switch to "laser manual" mode
        double lsr_mode_manual = 1.;
        dev->set_options(&opt_lsr_mode, 1, &lsr_mode_manual);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        dev->get_option_range(opt_lsr_pwr, min, max, step, def);

        for (double set_val = min; set_val <= max; set_val += step)
        {
            dev->set_options(&opt_lsr_pwr, 1, &set_val);
            dev->get_options(&opt_lsr_pwr, 1, &res);
            REQUIRE(set_val == res);
        }

        // Reset laser control to original state
        dev->set_options(&opt_lsr_mode, 1, &lsr_mode_init);
    }
}

TEST_CASE("RS4XX Manual Gain Control", "[live] [RS4XX]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS4"));

    double set_val = 30., reset_val = 80., res = 0.;

    double gain_ctrl_init = 0.;
    rs::option opt = rs::option::color_gain;

    dev->get_options(&opt, 1, &gain_ctrl_init);
    INFO("Initial gain control value obtained from hardware is " << gain_ctrl_init);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Apply several iterations
    for (uint8_t i = 0; i < 1; i++)
    {
        dev->set_options(&opt, 1, &set_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(set_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        dev->set_options(&opt, 1, &reset_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(reset_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Revert to original value
    dev->set_options(&opt, 1, &gain_ctrl_init);
    dev->get_options(&opt, 1, &res);
    REQUIRE(gain_ctrl_init == res);
}

TEST_CASE("RS4XX Manual Exposure Control", "[live] [RS4XX]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS4"));

    double set_val = 1200., reset_val = 80., res = 0.;

    double exposure_ctrl_init = 0.;
    rs::option opt = rs::option::r200_lr_exposure;

    dev->get_options(&opt, 1, &exposure_ctrl_init);
    INFO("Initial exposure control value obtained from hardware is " << exposure_ctrl_init);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Apply several iterations
    for (uint8_t i = 0; i < 1; i++)
    {
        dev->set_options(&opt, 1, &set_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(set_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        dev->set_options(&opt, 1, &reset_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(reset_val == res);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Revert to original value
    dev->set_options(&opt, 1, &exposure_ctrl_init);
    dev->get_options(&opt, 1, &res);
    REQUIRE(exposure_ctrl_init == res);
}

TEST_CASE("RS4XX Transmit Raw Data", "[live] [RS4XX]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS4"));

    // Initialize and preset raw buffer
    rs_raw_buffer test_obj{};

    // List of test patterns for transmitting raw data. Represent "request buffer content -> response buffer size" structure
    std::vector< std::pair<std::vector<uint8_t>, unsigned short>> snd_rcv_patterns{
        { { 0x14, 0x0, 0xab, 0xcd, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } , 216},      // GVD 
        { { 0x14, 0x0, 0xab, 0xcd, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } , 216 },     // GVD 
    };

    for (auto test_pattern : snd_rcv_patterns)
    {
        assert((test_pattern.first.size() > 0) && (test_pattern.first.size() <=RAW_BUFFER_SIZE));
        assert((test_pattern.second >= 0) && (test_pattern.second <= RAW_BUFFER_SIZE));
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(test_pattern.first.begin(), test_pattern.first.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = test_pattern.first.size();

        // Check send/receive raw data opaque binary buffer to exchange data with device
        dev->transmit_raw_data(&test_obj);

        REQUIRE(test_obj.rcv_buffer_size == test_pattern.second);
        REQUIRE(test_obj.rcv_buffer_size <= RAW_BUFFER_SIZE);
    }
}

///////////////////////////////////
// Calibration information tests //
///////////////////////////////////
TEST_CASE("RS4XX Extrinsics", "[live] [RS4XX]")
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    // For each device
    for (int i = 0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        SECTION("Extrinsic transformation between DEPTH and INFRARED is Identity matrix")
        {
            rs_extrinsics extrin;
            rs_get_device_extrinsics(dev, RS_STREAM_DEPTH, RS_STREAM_INFRARED, &extrin, require_no_error());

            require_identity_matrix(extrin.rotation);
            require_zero_vector(extrin.translation);
        }
    }
}

TEST_CASE("RS4XX Intrinsic", "[live] [RS4XX]")
{
    // Require at least one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count > 0);

    // For each device
    for (int i = 0; i<device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, 0, require_no_error());
        REQUIRE(dev != nullptr);

        // Require that there are a nonzero amount of infrared modes
        const int depth_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_DEPTH, require_no_error());
        REQUIRE(depth_mode_count > 0);

        // For each streaming mode
        for (int j = 0; j<depth_mode_count; ++j)
        {
            int depth_width = 0, depth_height = 0, depth_framerate = 0; rs_format depth_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_DEPTH, j, &depth_width, &depth_height, &depth_format, &depth_framerate, require_no_error());

            rs_enable_stream(dev, RS_STREAM_DEPTH, depth_width, depth_height, depth_format, depth_framerate, require_no_error());

            rs_intrinsics depth_intrin = {};
            rs_get_stream_intrinsics(dev, RS_STREAM_DEPTH, &depth_intrin, require_no_error());
            REQUIRE(depth_intrin.ppx == Approx(depth_intrin.width/2).epsilon(depth_intrin.width*0.005));    /* PPx and PPy are within 0.5% from the resolution center (pixels) */
            REQUIRE(depth_intrin.ppy == Approx(depth_intrin.height / 2).epsilon(depth_intrin.height*0.01));
            REQUIRE(depth_intrin.fx != Approx(0.0));
            REQUIRE(depth_intrin.fy != Approx(0.0));
            REQUIRE(depth_intrin.model == RS_DISTORTION_BROWN_CONRADY);
            for (int k = 0; k<5; ++k) REQUIRE(depth_intrin.coeffs[k] == 0.0);

            rs_disable_stream(dev, RS_STREAM_DEPTH, require_no_error());
        }


        // Require that there are a nonzero amount of infrared modes
        const int infrared_mode_count = rs_get_stream_mode_count(dev, RS_STREAM_INFRARED, require_no_error());
        REQUIRE(depth_mode_count > 0);

        // For each streaming mode
        for (int j = 0; j<infrared_mode_count; ++j)
        {
            int infrared_width = 0, infrared_height = 0, infrared_framerate = 0; rs_format infrared_format = RS_FORMAT_ANY;
            rs_get_stream_mode(dev, RS_STREAM_INFRARED, j, &infrared_width, &infrared_height, &infrared_format, &infrared_framerate, require_no_error());

            // Intrinsic data is availalbe for Infrared Left Rectified format only (Y8)
            if (infrared_format == RS_FORMAT_Y8)
            {
                rs_enable_stream(dev, RS_STREAM_INFRARED, infrared_width, infrared_height, infrared_format, infrared_framerate, require_no_error());

                rs_intrinsics infrared_intrin = {};
                rs_get_stream_intrinsics(dev, RS_STREAM_INFRARED, &infrared_intrin, require_no_error());
                CHECK(infrared_intrin.ppx == Approx(infrared_intrin.width / 2).epsilon(infrared_intrin.width*0.005));    /* PPx and PPy are within 0.5% from the resolution center (pixels) */
                CHECK(infrared_intrin.ppy == Approx(infrared_intrin.height / 2).epsilon(infrared_intrin.height*0.01));
                CHECK(infrared_intrin.fx != Approx(0.0));
                CHECK(infrared_intrin.fy != Approx(0.0));
                CHECK(infrared_intrin.model == RS_DISTORTION_BROWN_CONRADY);
                for (int k = 0; k < 5; ++k) REQUIRE(infrared_intrin.coeffs[k] == 0.0);

                rs_disable_stream(dev, RS_STREAM_INFRARED, require_no_error());
            }
        }
    }
}

TEST_CASE("RS4XX Streaming Formats", "[live] [RS4XX]")
{
    // Require only one device to be plugged in
    safe_context ctx;
    const int device_count = rs_get_device_count(ctx, require_no_error());
    REQUIRE(device_count == 1);

    rs_device * dev = rs_get_device(ctx, 0, require_no_error());
    REQUIRE(dev != nullptr);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS4"));

    struct int2 { int x, y; };
    struct cam_mode { int2 dims; std::vector<int> fps; };
    static const cam_mode rs400_depth_ir_modes[] = {
        { { 1280, 720 },{ 6,15,30 } },
        { { 848, 480 },{ 6,15,30,60,120 } },
        { { 640, 480 },{ 6,15,30,60,120 } },
        { { 640, 360 },{ 6,15,30,60,120 } },
        { { 480, 270 },{ 6,15,30,60,120 } },
        { { 424, 240 },{ 6,15,30,60,120 } },
    };

    static const cam_mode rs400_calibration_modes[] = {
        { { 1920,1080 },{ 15,30 } },
        { { 960, 540 },{ 15,30 } },
    };

    std::vector<std::pair<int, int>> resolutions = { {424,240},{480,270},{640,360},{ 640,480 },{ 848,480 },{ 1280,720 } };

    for (auto &mode : rs400_depth_ir_modes)
    {
        for (auto &cur_fps : mode.fps)
        {
            std::stringstream ss; ss << "Streaming Depth Profile " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_DEPTH,  mode.dims.x,  mode.dims.y, RS_FORMAT_Z16, cur_fps } });

            ss.str(""); ss << "Streaming IR Profile Y8 " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_INFRARED,  mode.dims.x,  mode.dims.y, RS_FORMAT_Y8, cur_fps } });

            ss.str(""); ss << "Streaming IR Profile UYVY " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_INFRARED,  mode.dims.x,  mode.dims.y, RS_FORMAT_UYVY, cur_fps } });

            ss.str(""); ss << "Streaming IR Profile Y8I " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_INFRARED,  mode.dims.x,  mode.dims.y, RS_FORMAT_Y8, cur_fps },
                                  { RS_STREAM_INFRARED2,  mode.dims.x,  mode.dims.y, RS_FORMAT_Y8, cur_fps } });

            ss.str(""); ss << "Streaming Depth + Y8 " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_DEPTH,     mode.dims.x,  mode.dims.y, RS_FORMAT_Z16, cur_fps },
                                  { RS_STREAM_INFRARED,  mode.dims.x,  mode.dims.y, RS_FORMAT_Y8, cur_fps } });

            ss.str(""); ss << "Streaming Depth + IR(UYVY) " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_DEPTH,     mode.dims.x,  mode.dims.y, RS_FORMAT_Z16, cur_fps },
                                  { RS_STREAM_INFRARED,  mode.dims.x,  mode.dims.y, RS_FORMAT_UYVY, cur_fps } });

            ss.str(""); ss << "Streaming Depth + Y8I " << mode.dims.x << "X" << mode.dims.y << " at " << cur_fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_DEPTH,     mode.dims.x,  mode.dims.y, RS_FORMAT_Z16, cur_fps },
                                  { RS_STREAM_INFRARED,  mode.dims.x,  mode.dims.y, RS_FORMAT_Y8, cur_fps },
                                  { RS_STREAM_INFRARED2,  mode.dims.x,  mode.dims.y, RS_FORMAT_Y8, cur_fps } });
        }
    }

    for (auto &res : resolutions)
    {
        for (auto fps : { 6,15,30,60,120 })
        {
            if (((res.second == 480) && (fps > 60)) || ((res.second == 720) && (fps > 30))) return;   // Skip unsupported modes
            std::stringstream ss; ss << "Streaming Z16 " << res.first <<  "X" << res.second << " at " << fps << " fps";
            INFO(ss.str().c_str());
            test_streaming(dev, { { RS_STREAM_DEPTH, res.first, res.second, RS_FORMAT_Z16, fps } });
        }
    }
}
#endif /* #if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(DS5_TEST) ) */
