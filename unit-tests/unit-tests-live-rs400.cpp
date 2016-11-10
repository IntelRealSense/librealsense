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

    const std::map<std::string, std::vector<rs_option>> rs4xx_sku_options
    {
        { "Intel RealSense RS400p", { RS_OPTION_COLOR_GAIN, RS_OPTION_CT_AUTO_EXPOSURE_MODE, RS_OPTION_R200_LR_EXPOSURE, RS_OPTION_HARDWARE_LOGGER_ENABLED }},
        { "Intel RealSense RS410a", { RS_OPTION_COLOR_GAIN, RS_OPTION_CT_AUTO_EXPOSURE_MODE, RS_OPTION_R200_LR_EXPOSURE, RS_OPTION_HARDWARE_LOGGER_ENABLED,
                                      RS_OPTION_RS4XX_PROJECTOR_MODE, RS_OPTION_RS4XX_PROJECTOR_PWR }},
    };

    // For each device
    for (int i = 0; i < device_count; ++i)
    {
        rs_device * dev = rs_get_device(ctx, i, require_no_error());
        REQUIRE(dev != nullptr);

        auto it = rs4xx_sku_options.find(dev->get_name());
        REQUIRE(it != rs4xx_sku_options.end());

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

    double set_val = -1, res = -1;
    double exposure_ctrl_init{}, min{}, max{}, step{}, def{}, in_val{};
    rs::option opt = rs::option::r200_lr_exposure;

    dev->get_options(&opt, 1, &exposure_ctrl_init);
    INFO("Initial exposure control value obtained from hardware is " << exposure_ctrl_init);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Obtain range of valid values
    dev->get_option_range(opt, min, max, step, def);

    const uint8_t steps = 100;
    int test_step = int(max - min) / steps;

    // Scan the exposure range
    for (set_val = min; set_val < max; set_val+= test_step)
    {
        dev->set_options(&opt, 1, &set_val);
        dev->get_options(&opt, 1, &res);
        REQUIRE(set_val == res);
    }

    // Revert to original value
    dev->set_options(&opt, 1, &exposure_ctrl_init);
    dev->get_options(&opt, 1, &res);
    REQUIRE(exposure_ctrl_init == res);
}

TEST_CASE("RS4XX Auto Exposure Control", "[live] [RS4XX]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS4"));

    double set_val = 1, reset_val = 0, res = -1;
    double exposure_mode_init{}, min{}, max{}, step{}, def{}, in_val{};
    rs::option opt = rs::option::ct_auto_exposure_mode;

    dev->get_options(&opt, 1, &exposure_mode_init);
    INFO("Initial auto-exposure mode obtained from hardware is " << exposure_mode_init);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    dev->get_option_range(opt, min, max, step, def);

    dev->set_options(&opt, 1, &set_val);
    dev->get_options(&opt, 1, &res);
    REQUIRE(set_val == res);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    dev->set_options(&opt, 1, &reset_val);
    dev->get_options(&opt, 1, &res);
    REQUIRE(reset_val == res);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // Revert to original value
    dev->set_options(&opt, 1, &exposure_mode_init);
    dev->get_options(&opt, 1, &res);
    REQUIRE(exposure_mode_init == res);
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

    std::vector<uint8_t> uame_off{ 0x14, 0x0, 0xab, 0xcd, 0x2d, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<uint8_t> uame_on{ 0x14, 0x0, 0xab, 0xcd, 0x2d, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<uint8_t> gvd{ 0x14, 0x0, 0xab, 0xcd, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<uint8_t> hw_rst{ 0x14, 0x0, 0xab, 0xcd, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    // List of test patterns for transmitting raw data. Represent "request buffer content -> response buffer size" structure
    std::vector< std::pair<std::vector<uint8_t>, unsigned short>> snd_rcv_patterns{
        { gvd, 216 },       // Input buffer - > Expected result buffer lenght
        { uame_off, 4 },
        { uame_on, 4 },
        { hw_rst, 4 },
    };

    for (auto test_pattern : snd_rcv_patterns)
    {
        assert((test_pattern.first.size() > 0) && (test_pattern.first.size() <= RAW_BUFFER_SIZE));
        assert(test_pattern.second <= RAW_BUFFER_SIZE);
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(test_pattern.first.begin(), test_pattern.first.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = test_pattern.first.size();

        // Check send/receive raw data opaque binary buffer to exchange data with device
        dev->transmit_raw_data(&test_obj);

        REQUIRE(test_obj.rcv_buffer[0] == test_obj.snd_buffer[4]);
        REQUIRE(test_obj.rcv_buffer_size == test_pattern.second);
        REQUIRE(test_obj.rcv_buffer_size <= RAW_BUFFER_SIZE);
    }

}

TEST_CASE("RS4XX Advanced Mode Verification", "[live] [RS4XX]")
{
    rs::context ctx;
    REQUIRE(ctx.get_device_count() == 1);

    rs::device * dev = ctx.get_device(0);
    REQUIRE(nullptr != dev);

    std::string name = dev->get_name();
    REQUIRE(std::string::npos != name.find("Intel RealSense RS4"));

    // Initialize and preset raw buffer
    rs_raw_buffer test_obj{};

    std::vector<uint8_t> uame_off{ 0x14, 0x0, 0xab, 0xcd, 0x2d, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<uint8_t> uame_on{ 0x14, 0x0, 0xab, 0xcd, 0x2d, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<uint8_t> gvd{ 0x14, 0x0, 0xab, 0xcd, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::vector<uint8_t> hw_rst{ 0x14, 0x0, 0xab, 0xcd, 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


    SECTION("Restart into Advanced Mode")
    {
        // Enter advanced mode
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(uame_on.begin(), uame_on.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = uame_on.size();

        dev->transmit_raw_data(&test_obj);

        // Reset FW to re-enumerate device
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(hw_rst.begin(), hw_rst.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = hw_rst.size();
        dev->transmit_raw_data(&test_obj);

        // Wait for device to re-enumerate
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    SECTION("Select and try to activate advanced mode profile")
    {
        try
        {
            // Direct verification
            if (dev->supports(rs::camera_info::advanced_mode))
                REQUIRE(std::string(dev->get_info(rs::camera_info::advanced_mode)) == std::string("Enabled"));
            // Indirect verification
            dev->enable_stream(rs::stream::infrared, 1280, 720, rs::format::y8, 30);
            dev->enable_stream(rs::stream::infrared2,1280, 720, rs::format::y8, 30);
            dev->start();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            dev->stop();
            REQUIRE(true);  // the flow have reached the designed milestone
            INFO("Test passed - advanced mode profiles are available, the way it should be");
        }
        catch (const std::runtime_error& e)
        {
            INFO(e.what());
            FAIL("Test failed - advanced mode profiles are not available in Advanced mode");;
        }
    }

    SECTION("Restart into Operational Mode")
    {
        // De-activate advanced mode
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(uame_off.begin(), uame_off.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = uame_off.size();

        dev->transmit_raw_data(&test_obj);

        // Reset FW to re-enumerate device
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(hw_rst.begin(), hw_rst.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = hw_rst.size();
        dev->transmit_raw_data(&test_obj);

        // Wait for device to re-enumerate
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    SECTION("Advanced mode - Negative Test")
    {
        try
        {
            // Direct verification
            if (dev->supports(rs::camera_info::advanced_mode))
                REQUIRE(std::string(dev->get_info(rs::camera_info::advanced_mode)) == std::string("Disabled"));
            dev->enable_stream(rs::stream::infrared, 1920, 1080, rs::format::y8, 30);
            dev->enable_stream(rs::stream::infrared2, 1920, 1080, rs::format::y8, 30);
            dev->start();
            std::this_thread::sleep_for(std::chrono::seconds(2));
            dev->stop();
            FAIL("Test failed - advanced mode profile was enable when it shouldn't have");
        }
        catch (const std::runtime_error& )
        {
            REQUIRE(true); // The flow must reach this exception handler
            INFO("Test passed - advanced mode profiles are not available, the way it should be");
        }
    }

    SECTION("Revert to advanced Mode")
    {
        // Enter advanced mode
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(uame_on.begin(), uame_on.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = uame_on.size();
        dev->transmit_raw_data(&test_obj);

        // Reset FW to re-enumerate device
        memset(&test_obj, 0, sizeof(rs_raw_buffer));
        std::copy(hw_rst.begin(), hw_rst.end(), test_obj.snd_buffer);
        test_obj.snd_buffer_size = hw_rst.size();
        dev->transmit_raw_data(&test_obj);

        // Wait for device to re-enumerate
        std::this_thread::sleep_for(std::chrono::seconds(3));
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
}
#endif /* #if !defined(MAKEFILE) || ( defined(LIVE_TEST) && defined(DS5_TEST) ) */
