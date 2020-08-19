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

#ifdef WIN32
#  define popen _popen
#  define pclose _pclose
#endif

#define TIME_TEST_SEC 10

std::string server_log;

std::string get_env(std::string name, std::string def) {
    static std::map<std::string, std::string> env;

    if (env.find(name) == env.end()) {
        char* val = std::getenv(name.c_str());
        if (val) {
            env.insert(std::pair<std::string, std::string>(name, val));
        }
        else {
            env.insert(std::pair<std::string, std::string>(name, def));
        }
    }

    return env[name];
}

std::string server_address() {
    return get_env("SERVER_ADDRESS", "127.0.0.1");
}

std::string user_name() {
    return get_env("USER_NAME", "pi");
}

std::string server_runcmd() {
    return get_env("SERVER_RUNCMD", "\"cd /tmp/lrs-net && /usr/bin/sudo ./rs-server -i " + server_address() + "\"");
}

/*
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
*/

std::string exec_cmd(std::string command) {
    char buffer[1024];
    std::string result = "";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while ((fgets(buffer, sizeof buffer, pipe) != NULL)) {
            result += buffer;
        }
    }
    catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);

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
    stop_server();

    std::thread t([&]() {
        server_log = ssh_cmd(server_runcmd());
    });
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return t;
}

TEST_CASE("Basic Pipeline Depth Streaming", "[net]") {
    rs2::context ctx;
    std::thread server;
    rs2::pipeline p;
    rs2::frame depth;
    int total_frames = 0;
    int width = 0;
    int height = 0;
    int fps = 0;

    REQUIRE_NOTHROW(server = start_server());
    REQUIRE_NOTHROW(rs2::net_device(server_address().c_str()).add_to(ctx));

    // Create a Pipeline - this serves as a top-level API for streaming and processing frames
    REQUIRE_NOTHROW(p = rs2::pipeline(ctx));

    // Configure and start the pipeline
    REQUIRE_NOTHROW(p.start());

    time_t last, start = time(nullptr);
    do {
        // Block program until frames arrive
        rs2::frameset frames;
        REQUIRE_NOTHROW(frames = p.wait_for_frames());

        // Try to get a frame of a depth image
        if (width == 0) REQUIRE_NOTHROW(width = frames.get_depth_frame().get_width());
        if (height == 0) REQUIRE_NOTHROW(height = frames.get_depth_frame().get_height());

        // Get the depth frame's dimensions
        total_frames++;

        last = time(nullptr);
    } while (last - start < TIME_TEST_SEC);
    fps = total_frames / TIME_TEST_SEC;

    REQUIRE_NOTHROW(stop_server());
    REQUIRE_NOTHROW(server.join());

    // Report
    CAPTURE(width, height, fps, total_frames, server_log);
    REQUIRE(total_frames > 0);
}

int frames = 0;
auto callback = [&](const rs2::frame &frame) {
    frames++;
};

TEST_CASE("All profiles Streaming", "[net]") {
    std::thread server;
    rs2::device dev;
    std::vector<rs2::sensor> sensors;
    std::vector<rs2::stream_profile> profiles;

    REQUIRE_NOTHROW(server = start_server());
    REQUIRE_NOTHROW(dev = rs2::net_device(server_address()));
    REQUIRE_NOTHROW(sensors = dev.query_sensors());
    REQUIRE(sensors.size() > 0);
    for (int i = 0; i < sensors.size(); i++) {
        REQUIRE_NOTHROW(profiles = sensors[i].get_stream_profiles());
        for (rs2::stream_profile profile : profiles) {
            frames = 0;
            REQUIRE_NOTHROW(sensors[i].open(profile));
            REQUIRE_NOTHROW(sensors[i].start(callback));
            std::this_thread::sleep_for(std::chrono::seconds(TIME_TEST_SEC));
            REQUIRE_NOTHROW(sensors[i].stop());
            REQUIRE_NOTHROW(sensors[i].close());

            rs2_stream stream = profile.stream_type();
            int fps = profile.fps();
            int width = ((rs2::video_stream_profile)profile).width();
            int height = ((rs2::video_stream_profile)profile).height();
            int expected_frames = TIME_TEST_SEC * profile.fps();
            int received_frames = frames;
            float drop = ((float)expected_frames / (float)received_frames - 1) * 100;
            CAPTURE(stream, fps, width, height, expected_frames, received_frames, drop, server_log);
            REQUIRE(received_frames == Approx(expected_frames).epsilon(0.1)); // 10%

            /*
            std::cerr << "====================================\n";
            std::cerr << "Stream   : " << stream << std::endl;
            std::cerr << "FPS      : " << fps << std::endl;
            std::cerr << "Width    : " << width << std::endl;
            std::cerr << "Height   : " << height << std::endl;
            std::cerr << "Expected : " << expected_frames << std::endl;
            std::cerr << "Received : " << received_frames << std::endl;
            std::cerr << "Drop %   : " << drop << std::endl;
            */
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    REQUIRE_NOTHROW(stop_server());
    REQUIRE_NOTHROW(server.join());
}

