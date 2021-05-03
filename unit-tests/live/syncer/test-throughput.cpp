// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device D400*


#define CATCH_CONFIG_MAIN
#include <stdlib.h> 
#include "../../catch.h"
#include "../../unit-tests-common.h"

using namespace rs2;
constexpr int RECEIVE_FRAMES_TIME = 5;

TEST_CASE("Syncer dynamic FPS - throughput test", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    auto list = ctx.query_devices();
    REQUIRE(list.size());
    auto dev = list.front();
    auto sensors = dev.query_sensors();
    REQUIRE(dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE));
    std::string device_type = list.front().get_info(RS2_CAMERA_INFO_PRODUCT_LINE);
    int width = 848;
    int height = 480;
    if (device_type == "L500")
        width = 640;

    sensor rgb_sensor;
    sensor ir_sensor;
    std::vector<rs2::stream_profile > rgb_stream_profile;
    std::vector<rs2::stream_profile > ir_stream_profile;

    for (auto& s : sensors)
    {
        auto info = std::string(s.get_info(RS2_CAMERA_INFO_NAME));
        auto stream_profiles = s.get_stream_profiles();
        for (auto& sp : stream_profiles)
        {
            auto vid = sp.as<rs2::video_stream_profile>();
            if (!(vid.width() == width && vid.height() == height && vid.fps() == 60)) continue;
            if (sp.stream_type() == RS2_STREAM_COLOR && sp.format() == RS2_FORMAT_RGB8)
                rgb_stream_profile.push_back(sp);
            if (sp.stream_type() == RS2_STREAM_INFRARED)
                ir_stream_profile.push_back(sp);
        }
        if (info == "RGB Camera")
            rgb_sensor = s;

        if (info == "Stereo Module")
            ir_sensor = s;
    }

    typedef enum configuration
    {
        IR_ONLY,
        IR_RGB_EXPOSURE,
        STOP
    }configuration;

    configuration tests[2] = { IR_ONLY, IR_RGB_EXPOSURE };
    std::map<long long, std::vector<unsigned long long>> frames_num_info; // {fps: frames}
    std::map < configuration, rs2_metadata_type> actual_fps;
    std::map<int, std::string> stream_names;
    rs2::syncer sync;
    double prev_fps = 0;

    auto process_frame = [&](const rs2::frame& f)
    {
        auto stream_type = std::string(f.get_profile().stream_name());
        // Only IR fps is relevant for this test, IR1 and IR2 have same fps so it is enough to get only one of them
        if (stream_type != "Infrared 1")
            return;
        auto frame_num = f.get_frame_number();
        auto fps = f.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS);
        if (!frames_num_info[fps].empty() && frames_num_info[fps].back() == frame_num) // check if frame is already processed
            return;
        frames_num_info[fps].push_back(frame_num);
    };

    auto validate_ratio = [&](uint16_t delta, configuration test)
    {
        for (auto f : frames_num_info)
        {
            std::cout << "Infrared 1 : " << f.first << " fps, " << f.second.size() << " frames" << std::endl;
            if (test == STOP) // after changing exposure to 18000, fps should be changed 60 -> 30
            {
                double ratio = (double)f.first / prev_fps;
                REQUIRE(ratio < 0.6);
            }
            prev_fps = (double)f.first;
            double calc_fps = f.second.size() / delta;
            double fps_ratio = calc_fps/ f.first;
            CAPTURE(calc_fps, f.second.size(), delta, f.first);
            REQUIRE(fps_ratio > 0.8);
            
        }
    };

    for (auto& test : tests)
    {
        CAPTURE(test);
        frames_num_info.clear();
        ir_sensor.set_option(RS2_OPTION_EXPOSURE, 1);
        ir_sensor.open(ir_stream_profile); // ir streams in all configurations
        ir_sensor.start(sync);

        if (test == IR_RGB_EXPOSURE)
        {
            rgb_sensor.open(rgb_stream_profile);
            rgb_sensor.start(sync);
        }

        std::cout << "==============================================" << std::endl;
        std::cout << "Configuration " << test << std::endl << std::endl;

        auto t_start = std::chrono::system_clock::now();
        auto t_end = std::chrono::system_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();
        while (delta < RECEIVE_FRAMES_TIME)
        {
            auto fs = sync.wait_for_frames();
            for (const rs2::frame& ff : fs)
            {
                process_frame(ff);
            }
            t_end = std::chrono::system_clock::now();
            delta = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();

            if (test == IR_RGB_EXPOSURE && delta >= RECEIVE_FRAMES_TIME) // modify exposure 5 seconds after start streaming ( according to repro description )
            {
                validate_ratio(delta, test);
                test = STOP;
                delta = 0;
                frames_num_info.clear();
                t_start = std::chrono::system_clock::now();
                ir_sensor.set_option(RS2_OPTION_EXPOSURE, 18000); // set exposure value to x > 1000/fps
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // wait 200 msec to process FW command
            }
        }
        validate_ratio(delta, test);

        ir_sensor.stop();
        ir_sensor.close();
        if (test == IR_RGB_EXPOSURE)
        {
            rgb_sensor.stop();
            rgb_sensor.close();
        }
    }
}