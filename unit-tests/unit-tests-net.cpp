// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any number and combination of RealSense cameras, including R200 and F200 //
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>  
#include <stdlib.h> 

#include "unit-tests-common.h"

#include <librealsense2/hpp/rs_types.hpp>
#include <librealsense2/hpp/rs_frame.hpp>
#include <librealsense2-net/rs_net.hpp>

#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <thread>

#include <iostream>
#include <fstream>

using namespace rs2;

#define TIME_TEST_SEC 10

std::string server_log;

std::ofstream fuflog("c:/temp/fuflog.log");

std::string server_address() {
    static std::string server_address = "";

    if (server_address.empty()) {
        const char* srv_addr = std::getenv("SERVER_ADDRESS");
        if (srv_addr) {
            server_address = srv_addr;
        }
        else {
            server_address = "127.0.0.1";
        }
    }

    return server_address;
}

std::string user_name() {
    static std::string user_name = "";

    if (user_name.empty()) {
        const char* usr_name = std::getenv("USER_NAME");
        if (usr_name) {
            user_name = usr_name;
        }
        else {
            user_name = "pi";
        }
    }

    return user_name;
}

std::string exec_cmd(std::string command) {
    char buffer[128];
    std::string result = "";

    fuflog << "[LRS-NET TEST] Command to run: " << command << std::endl;

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while ((fgets(buffer, sizeof buffer, pipe) != NULL)) {
            result += buffer;
        }
    } catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);

    return result;
}

std::string ssh_cmd(std::string command) {
    return exec_cmd("ssh " + user_name() + "@" + server_address() + " " + command);
}

void stop_server() {
    ssh_cmd("killall    rs-server");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ssh_cmd("killall -9 rs-server");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return;
}

std::thread start_server() {
    fuflog << "[LRS-NET TEST] Server at " << server_address() << " username is " << user_name() << std::endl;

    stop_server();

    std::thread t([&]() {
        fuflog << "[LRS-NET TEST] Starting remote server" << std::endl;
        server_log = ssh_cmd("/tmp/rs-server -i "  + server_address());
        fuflog << "[LRS-NET TEST] Remote server exited" << std::endl;
    });
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return t;
}

TEST_CASE("Basic Pipeline Depth Streaming", "[net]") {
    fuflog << "[LRS-NET TEST] " << Catch::getResultCapture().getCurrentTestName() << std::endl;

    rs2::context ctx;
    std::thread server = start_server();
    fuflog << "[LRS-NET TEST] Server started" << std::endl;

    rs2::net_device dev(server_address().c_str());
    dev.add_to(ctx);

    // Create a Pipeline - this serves as a top-level API for streaming and processing frames
    rs2::pipeline p(ctx);

    // Configure and start the pipeline
    p.start();

    uint32_t depth_frames = 0;

    time_t last, start = time(nullptr);
    float width = 0;
    float height = 0;
    do
    {
        // Block program until frames arrive
        rs2::frameset frames;
        try {
            frames = p.wait_for_frames();
        }
        catch (...) {
            fuflog << "[LRS-NET TEST] Frames didn't arrive" << std::endl;
            break;
        }

        // Try to get a frame of a depth image
        rs2::depth_frame depth = frames.get_depth_frame();

        // Get the depth frame's dimensions
        width = depth.get_width();
        height = depth.get_height();

        depth_frames++;

        last = time(nullptr);
    } while (last - start < TIME_TEST_SEC);

    stop_server();
    fuflog << "[LRS-NET TEST] Server stopped" << std::endl;

    fuflog << "[LRS-NET TEST] Waiting for remote server" << std::endl;
    server.join();

    // Report
    fuflog << "[LRS-NET TEST] ============================" << std::endl;
    fuflog << "[LRS-NET TEST] Stream Width  : " << width << std::endl;
    fuflog << "[LRS-NET TEST] Stream Height : " << height << std::endl;
    fuflog << "[LRS-NET TEST] Stream FPS    : " << depth_frames / TIME_TEST_SEC << std::endl;
    fuflog << "[LRS-NET TEST] Total frames  : " << depth_frames << std::endl;
    fuflog << "[LRS-NET TEST] Server log    :" << std::endl << server_log;
    fuflog << "[LRS-NET TEST] ============================" << std::endl;
    fuflog << "[LRS-NET TEST] " << std::endl;

    REQUIRE(depth_frames > 0);
}

int frames = 0;
auto callback = [&](const rs2::frame &frame) {
    frames++;
    if (frames % 10 == 0) fuflog << "[LRS-NET TEST] Got the frame: " << frames << std::endl;
};

TEST_CASE("All profiles Streaming", "[net]") {
    fuflog << "[LRS-NET TEST] " << Catch::getResultCapture().getCurrentTestName() << std::endl;

    std::thread server = start_server();
    fuflog << "[LRS-NET TEST] Server started" << std::endl;

    rs2::device dev = rs2::net_device((server_address()).c_str());

    auto sensors = dev.query_sensors();
    for (int i = 0; i < sensors.size(); i++) {
        auto profiles = sensors[i].get_stream_profiles();
        for (rs2::stream_profile profile : profiles) {
            fuflog << "[LRS-NET TEST] ====================================" << std::endl;
            fuflog << "[LRS-NET TEST] Stream Type: " << profile.stream_type() << std::endl;
            fuflog << "[LRS-NET TEST] FPS      : " << profile.fps() << std::endl;
            fuflog << "[LRS-NET TEST] Width    : " << ((rs2::video_stream_profile)profile).width() << std::endl;
            fuflog << "[LRS-NET TEST] Height   : " << ((rs2::video_stream_profile)profile).height() << std::endl;

            fuflog << "[LRS-NET TEST] " << std::endl;
            frames = 0;
            REQUIRE_NOTHROW([&]() {
                sensors[i].open(profile);
                sensors[i].start(callback);
                std::this_thread::sleep_for(std::chrono::seconds(TIME_TEST_SEC));
                sensors[i].stop();
                sensors[i].close();
            }());
            fuflog << "[LRS-NET TEST] " << std::endl;

            int expected_frames = TIME_TEST_SEC * profile.fps();
            int received_frames = frames;
            float frame_range = (float)received_frames / (float)expected_frames;
            fuflog << "[LRS-NET TEST] Expected : " << expected_frames << std::endl;
            fuflog << "[LRS-NET TEST] Received : " << received_frames << std::endl;
            fuflog << "[LRS-NET TEST] Range    : " << frame_range << std::endl;
            fuflog << "[LRS-NET TEST] ====================================" << std::endl;

            Approx target = Approx(expected_frames).epsilon(0.1); // 10%
            REQUIRE(received_frames == target);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        } //end for profile
    }

    stop_server();
    fuflog << "[LRS-NET TEST] Server stopped" << std::endl;

    fuflog << "[LRS-NET TEST] Waiting for remote server" << std::endl;
    server.join();

    fuflog << "[LRS-NET TEST] ============================" << std::endl;
    fuflog << "[LRS-NET TEST] Server log    :" << std::endl << server_log;

    fuflog << "[LRS-NET TEST] " << std::endl;
}

