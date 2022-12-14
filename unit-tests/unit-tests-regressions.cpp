// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

////////////////////////////////////////////////
// Regressions tests against the known issues //
////////////////////////////////////////////////

#include <cmath>
#include "unit-tests-common.h"
#include "../include/librealsense2/rs_advanced_mode.hpp"
#include <librealsense2/hpp/rs_types.hpp>
#include <librealsense2/hpp/rs_frame.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rsutil.h>

using namespace rs2;

TEST_CASE("DSO-14512", "[live]")
{
    {
        rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG,"lrs_log.txt");
        //rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
        rs2::context ctx;
        if (make_context(SECTION_FROM_TEST_NAME, &ctx))
        {
            for (size_t iter = 0; iter < 10; iter++)
            {
                std::vector<rs2::device> list;
                REQUIRE_NOTHROW(list = ctx.query_devices());
                REQUIRE(list.size());

                auto dev = list[0];
                CAPTURE(dev.get_info(RS2_CAMERA_INFO_NAME));
                disable_sensitive_options_for(dev);

                std::mutex m;
                int fps = is_usb3(dev) ? 30 : 15; // In USB2 Mode the devices will switch to lower FPS rates

                for (auto i = 0; i < 1000; i++)
                {
                    std::map<std::string, size_t> frames_per_stream{};
                    std::map<std::string, size_t> frames_indx_per_stream{};
                    std::vector<std::string> drop_descriptions;

                    auto profiles = configure_all_supported_streams(dev, 640, 480, fps);
                    size_t drops_count=0;

                    for (auto s : profiles.first)
                    {
                        REQUIRE_NOTHROW(s.start([&m, &frames_per_stream,&frames_indx_per_stream,&drops_count,&drop_descriptions](rs2::frame f)
                            {
                                std::lock_guard<std::mutex> lock(m);
                                auto stream_name = f.get_profile().stream_name();
                                auto fn = f.get_frame_number();

                                if (frames_per_stream[stream_name])
                                {
                                    auto prev_fn = frames_indx_per_stream[stream_name];
                                    if ((fn = prev_fn) != 1)
                                    {
                                        drops_count++;
                                        std::stringstream s;
                                        s << "Frame drop was recognized for " << stream_name<< " jumped from " << prev_fn << " to " << fn;
                                        drop_descriptions.emplace_back(s.str().c_str());
                                    }
                                }
                                ++frames_per_stream[stream_name];
                                frames_indx_per_stream[stream_name] = fn;
                            }));
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(60));
                    // Stop & flush all active sensors. The separation is intended to semi-confirm the FPS
                    for (auto s :  profiles.first)
                        REQUIRE_NOTHROW(s.stop());
                    for (auto s : profiles.first)
                        REQUIRE_NOTHROW(s.close());

                    // Verify frames arrived for all the profiles specified
                    std::stringstream active_profiles, streams_arrived;
                    active_profiles << "Profiles requested : " << profiles.second.size() << std::endl;
                    for (auto& pf : profiles.second)
                        active_profiles << pf << std::endl;
                    streams_arrived << "Streams recorded : " << frames_per_stream.size() << std::endl;
                    for (auto& frec : frames_per_stream)
                        streams_arrived << frec.first << ": frames = " << frec.second << std::endl;

                    CAPTURE(active_profiles.str().c_str());
                    CAPTURE(streams_arrived.str().c_str());
                    REQUIRE(profiles.second.size() == frames_per_stream.size());
                    std::stringstream s;
                    s << "Streaming cycle " << i << " iteration " << iter << " completed,\n"
                        << active_profiles.str() << std::endl << streams_arrived.str() << std::endl;
                    WARN(s.str().c_str());

                    if (drops_count)
                    {
                        std::stringstream s;
                        for (auto& str : drop_descriptions)
                        {
                            s << str << std::endl;
                        }
                        WARN("\n\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!! " << drops_count << " Drop were identified !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
                        WARN(s.str().c_str());
                        REQUIRE(false);
                    }
                }

                if (dev.is<rs400::advanced_mode>())
                {
                    auto advanced = dev.as<rs400::advanced_mode>();
                    if (advanced.is_enabled())
                    {
                        std::cout << "Iteration " << iter << " ended, resetting device..." << std::endl;
                        advanced.hardware_reset();
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                    }
                }
                else
                {
                    FAIL("Device doesn't support AdvancedMode API");
                }
            }
        }
    }
}


TEST_CASE("Frame Drops", "[live]"){
    {
        rs2::context ctx;
        std::condition_variable cv;
        std::mutex m;

        if (make_context(SECTION_FROM_TEST_NAME, &ctx))
        {
            //rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
            rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG,"lrs_frame_drops_repro.txt");

            for (size_t iter = 0; iter < 1000; iter++)
            {
                std::vector<rs2::device> list;
                REQUIRE_NOTHROW(list = ctx.query_devices(RS2_PRODUCT_LINE_ANY));
                REQUIRE(list.size());

                auto dev = list[0];
                bool is_l500_device = false;
                if (dev.supports(RS2_CAMERA_INFO_PRODUCT_LINE) &&
                    std::string(dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE)) == "L500")
                {
                    is_l500_device = true;
                }

                CAPTURE(dev.get_info(RS2_CAMERA_INFO_NAME));
                disable_sensitive_options_for(dev);

                std::mutex m;
                size_t drops_count=0;
                bool all_streams = true;
                int fps = is_usb3(dev) ? 60 : 15; // In USB2 Mode the devices will switch to lower FPS rates
                if (is_l500_device)
                    fps = 30;
                float interval_msec = 1000.f / fps;

                for (auto i = 0; i < 200; i++)
                {
                    std::map<std::string, size_t> frames_per_stream{};
                    std::map<std::string, size_t> last_frame_per_stream{};
                    std::map<std::string, double> last_frame_ts_per_stream{};
                    std::map<std::string, rs2_timestamp_domain> last_ts_domain_per_stream{};
                    std::vector<std::string> drop_descriptions;
                    bool iter_finished  =false;

                    int width = is_l500_device ? 640 : 848;
                    int height = 480;
                    auto profiles = configure_all_supported_streams(dev, width, height, fps);
                    //for l500 - auto profiles = configure_all_supported_streams(dev, 640, 480, fps);
                    drops_count=0;

                    auto start_time = std::chrono::high_resolution_clock::now();
                    std::stringstream ss;
                    ss << "Iteration " << i << " started, time =  " << std::dec << start_time.time_since_epoch().count() << std::endl;
                    std::string syscl = "echo \"timecheck: $(date +%s.%N) = $(date +%FT%T,%N)\" | sudo tee /dev/kmsg";
                    auto r = system(syscl.c_str());
                    ss << "sys call result = " << r;
                    rs2::log(RS2_LOG_SEVERITY_INFO,ss.str().c_str());
                    std::cout << ss.str().c_str() << std::endl;
                    for (auto s : profiles.first)
                    {
                        REQUIRE_NOTHROW(s.start([&m, &frames_per_stream,&last_frame_per_stream,&last_frame_ts_per_stream,&last_ts_domain_per_stream,&drops_count,
                                                &drop_descriptions,&cv,&all_streams,&iter_finished,start_time,profiles,interval_msec,syscl](rs2::frame f)
                            {
                                auto time_elapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time);
                                auto stream_name = f.get_profile().stream_name();
                                auto fn = f.get_frame_number();
                                auto ts = f.get_timestamp();
                                auto ts_dom = f.get_frame_timestamp_domain();

                                if (frames_per_stream[stream_name])
                                {
                                    auto prev_fn = last_frame_per_stream[stream_name];
                                    auto prev_ts = last_frame_ts_per_stream[stream_name];
                                    auto prev_ts_dom = last_ts_domain_per_stream[stream_name];
                                    auto delta_t = ts - prev_ts;
                                    auto min_delay = interval_msec*3.5;
                                    auto max_delay = interval_msec*20;

                                    auto arrival_time = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time);
                                    // Skip events during the first 2 second
                                    if (time_elapsed.count() > 2000)
                                    {
                                        if (RS2_FORMAT_MOTION_XYZ32F != f.get_profile().format())
                                        {
                                            if (/*!(fn > prev_fn) ||*/
                                                 ((delta_t >= min_delay) && (RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME !=ts_dom) && (ts_dom ==prev_ts_dom)))
                                            {
                                                if ((fn - prev_fn) > 1)
                                                    drops_count++;
                                                std::stringstream s;
                                                s << "Frame drop was recognized for " << stream_name << " jump from fn " << prev_fn << "  to fn " << fn
                                                  << " from " << std::fixed << std::setprecision(3) << prev_ts << " to "
                                                  << ts << " while expected ts = " << (prev_ts + interval_msec)
                                                  << ", ts domain " << ts_dom
                                                  << " time elapsed: " << time_elapsed.count()/1000.f << " sec, host time: "
                                                  << std::chrono::high_resolution_clock::now().time_since_epoch().count();
                                                if (f.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
                                                    s << " hw ts: " << f.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP) << " = 0x"
                                                      << std::hex << f.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP) << std::dec;
                                                WARN(s.str().c_str());
                                                if ((delta_t < max_delay) || (fabs(fn - prev_fn)< 20)) // Skip inconsistent frame numbers due to intermittent metadata
                                                {
                                                    std::cout << "Frame interval = " << delta_t << ", max = " << max_delay << std::endl;
                                                    std::cout << "fn = " << fn << ", prev_fn = " << prev_fn << std::endl;
                                                    system(syscl.c_str());
                                                    exit(1);
                                                }
                                                drop_descriptions.emplace_back(s.str().c_str());
                                            }
                                        }
//                                        if (profiles.second.size() != frames_per_stream.size()) // Some streams failed to start
//                                        {
//                                            all_streams = false;
//                                        }
                                    }

                                    // Status prints
                                    auto sec_now = int(arrival_time.count()) / 1000;
                                    if (sec_now && (sec_now != (int(time_elapsed.count())/ 1000)) )
                                        std::cout << "Test runs for " << int(arrival_time.count()) / 1000 << " seconds" << std::endl;
                                    // Every 10th second print number of frames arrived
                                    if ((int(arrival_time.count()) / 10000) != (int(time_elapsed.count())/ 10000) )
                                    {
                                        std::stringstream s;
                                        s << "Frames arrived: ";
                                        for (const auto& entry : frames_per_stream)
                                            s << entry.first << " : " << entry.second << ", ";
                                        s << std::endl;
                                        std::cout << s.str().c_str();
                                    }

                                    // Stop test iteration after 20 sec
                                    if (arrival_time.count() > 20000)
                                    {
                                        std::lock_guard<std::mutex> lock(m);
                                        iter_finished = true;
                                        cv.notify_all();
                                    }
                                    time_elapsed = arrival_time;
                                }
                                ++frames_per_stream[stream_name];
                                last_frame_per_stream[stream_name] = fn;
                                last_frame_ts_per_stream[stream_name] = ts;
                                {
                                    std::lock_guard<std::mutex> lock(m);
                                    if (drops_count || (!all_streams))
                                        cv.notify_all();
                                }
                            }));
                    }

                    // block till error is reproduced
                    {
                        std::unique_lock<std::mutex> lk(m);
                        cv.wait(lk, [&drops_count,&all_streams,&iter_finished]{return (drops_count>0 || (!all_streams) || iter_finished);});
                        //std::this_thread::sleep_for(std::chrono::milliseconds(200)); // add some sleep to record additional errors, if any
                    }
                    std::cout << "Drops = " << drops_count << ", All streams valid = " << int(all_streams) << " iter completed = " << int(iter_finished) << std::endl;
                    // Stop & flush all active sensors. The separation is intended to semi-confirm the FPS
                    for (auto s : profiles.first)
                        REQUIRE_NOTHROW(s.stop());
                    for (auto s : profiles.first)
                        REQUIRE_NOTHROW(s.close());

                    // Verify frames arrived for all the profiles specified
                    std::stringstream active_profiles, streams_arrived;
                    active_profiles << "Profiles requested : " << profiles.second.size() << std::endl;
                    for (auto& pf : profiles.second)
                        active_profiles << pf << std::endl;
                    streams_arrived << "Streams recorded : " << frames_per_stream.size() << std::endl;
                    for (auto& frec : frames_per_stream)
                        streams_arrived << frec.first << ": frames = " << frec.second << std::endl;

                    CAPTURE(active_profiles.str().c_str());
                    CAPTURE(streams_arrived.str().c_str());

                    std::stringstream s;
                    s << "Streaming cycle " << i << " iteration " << iter << " completed,\n"
                        << active_profiles.str() << std::endl << streams_arrived.str() << std::endl;
                    WARN(s.str().c_str());

                    if (drops_count)
                    {
                        std::stringstream s;
                        for (auto& str : drop_descriptions)
                        {
                            s << str << std::endl;
                        }
                        WARN("\n!!! " << drops_count << " Drop were identified !!!\n");
                        WARN(s.str().c_str());
                    }

                    REQUIRE(profiles.second.size() == frames_per_stream.size());
                    //REQUIRE( drops_count == 0);
                    REQUIRE( all_streams);
                }

                if (dev.is<rs400::advanced_mode>())
                {
                    auto advanced = dev.as<rs400::advanced_mode>();
                    if (advanced.is_enabled())
                    {
                        std::cout << "Iteration " << iter << " ended, resetting device..." << std::endl;
                        advanced.hardware_reset();
                        std::this_thread::sleep_for(std::chrono::seconds(3));
                    }
                }
                else
                {
                    if(!is_l500_device)
                        FAIL("Device doesn't support AdvancedMode API");
                }
            }
        }
    }
}


TEST_CASE("DSO-15050", "[live]")
{
    {
        rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "lrs_log.txt");
        //rs2::log_to_console(RS2_LOG_SEVERITY_DEBUG);
        rs2::context ctx;
        if (make_context(SECTION_FROM_TEST_NAME, &ctx))
        {
            for (size_t iter = 0; iter < 10000; iter++)
            {
                std::vector<rs2::device> list;
                REQUIRE_NOTHROW(list = ctx.query_devices());
                REQUIRE(list.size());

                auto dev = std::make_shared<device>(list.front());

                disable_sensitive_options_for(*dev);
                CAPTURE(dev->get_info(RS2_CAMERA_INFO_NAME));
                std::string serial;
                REQUIRE_NOTHROW(serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));

                // Precausion
                for (auto&& sensor : dev->query_sensors())
                {
                    if (sensor.supports(RS2_OPTION_GLOBAL_TIME_ENABLED))
                        sensor.set_option(RS2_OPTION_GLOBAL_TIME_ENABLED, 0.f);
                }

                std::mutex m;
                int fps = is_usb3(*dev) ? 30 : 6; // In USB2 Mode the devices will switch to lower FPS rates

                for (auto i = 0; i < 1; i++)
                {
                    std::map<std::string, size_t> frames_per_stream{};
                    std::map<std::string, size_t> frames_indx_per_stream{};
                    std::vector<std::string> drop_descriptions;

                    auto profiles = configure_all_supported_streams(*dev, 1280, 720, fps);
                    size_t drops_count = 0;

                    for (auto s : profiles.first)
                    {
                        REQUIRE_NOTHROW(s.start([&m, &frames_per_stream, &frames_indx_per_stream, &drops_count, &drop_descriptions](rs2::frame f)
                            {
                                std::lock_guard<std::mutex> lock(m);
                                auto stream_name = f.get_profile().stream_name();
                                auto fn = f.get_frame_number();

                                if (frames_per_stream[stream_name])
                                {
                                    auto prev_fn = frames_indx_per_stream[stream_name];
                                    if ((fn = prev_fn) != 1)
                                    {
                                        drops_count++;
                                        std::stringstream s;
                                        s << "Frame drop was recognized for " << stream_name << " jumped from " << prev_fn << " to " << fn;
                                        drop_descriptions.emplace_back(s.str().c_str());
                                    }
                                }
                                ++frames_per_stream[stream_name];
                                frames_indx_per_stream[stream_name] = fn;
                            }));
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    // Stop & flush all active sensors. The separation is intended to semi-confirm the FPS
                    for (auto s : profiles.first)
                        REQUIRE_NOTHROW(s.stop());
                    for (auto s : profiles.first)
                        REQUIRE_NOTHROW(s.close());

                    // Verify frames arrived for all the profiles specified
                    std::stringstream active_profiles, streams_arrived;
                    active_profiles << "Profiles requested : " << profiles.second.size() << std::endl;
                    for (auto& pf : profiles.second)
                        active_profiles << pf << std::endl;
                    streams_arrived << "Streams recorded : " << frames_per_stream.size() << std::endl;
                    for (auto& frec : frames_per_stream)
                        streams_arrived << frec.first << ": frames = " << frec.second << std::endl;

                    CAPTURE(active_profiles.str().c_str());
                    CAPTURE(streams_arrived.str().c_str());
                    REQUIRE(profiles.second.size() == frames_per_stream.size());
                    std::stringstream s;
                    s << "Streaming cycle " << i << " iteration " << iter << " completed,\n"
                        << active_profiles.str() << std::endl << streams_arrived.str() << std::endl;
                    WARN(s.str().c_str());

                    if (drops_count)
                    {
                        std::stringstream s;
                        for (auto& str : drop_descriptions)
                        {
                            s << str << std::endl;
                        }
                        WARN("\n\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!! " << drops_count << " Drop were identified !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");
                        WARN(s.str().c_str());
                        //REQUIRE(false);
                    }
                }

                //forcing hardware reset to simulate device disconnection
                do_with_waiting_for_camera_connection(ctx, dev, serial, [&]()
                {
                    dev->hardware_reset();
                    std::cout << "Iteration " << iter << " ended, resetting device..." << std::endl;
                });
            }
        }
    }
}
