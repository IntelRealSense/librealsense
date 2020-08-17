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

    fuflog << "Command to run: " << command << "" << std::endl;

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
    return;
}

std::thread start_server() {
    fuflog << "Server at " << server_address() << " username is " << user_name() << std::endl;

    stop_server();

    std::thread t([&]() {
        fuflog << "Starting remote server" << std::endl;
        server_log = ssh_cmd("/tmp/rs-server -i "  + server_address());
        fuflog << "Remote server exited" << std::endl;
    });
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return t;
}

TEST_CASE("Basic Depth Streaming (10 sec)", "[net]") {
    rs2::context ctx;

    fuflog << "LRS-Net Streaming test" << std::endl;

    std::thread server = start_server();
    fuflog << "Server started" << std::endl;

    rs2::net_device dev(server_address());
    fuflog << "Device created" << std::endl;
    dev.add_to(ctx);
    fuflog << "Added to context" << std::endl;

    // Create a Pipeline - this serves as a top-level API for streaming and processing frames
    rs2::pipeline p(ctx);
    fuflog << "Pipeline created" << std::endl;

    // Configure and start the pipeline
    p.start();
    fuflog << "Pipeline started" << std::endl;

    uint32_t depth_frames = 0;

    time_t last, start = time(nullptr);
    float width, height;
    do 
    {
        // Block program until frames arrive
        rs2::frameset frames = p.wait_for_frames();

        // Try to get a frame of a depth image
        rs2::depth_frame depth = frames.get_depth_frame();

        // Get the depth frame's dimensions
        width = depth.get_width();
        height = depth.get_height();

        depth_frames++;

        last = time(nullptr);
    } while (last - start < 10);

    try {
        p.stop();
    }
    catch (...) {
        fuflog << "Cannot stop pipeline" << std::endl;
    }
    fuflog << "Pipeline stopped" << std::endl;
    stop_server();
    fuflog << "Server stopped" << std::endl;

    fuflog << "Waiting for remote server" << std::endl;
    server.join();

    // Report
    fuflog << "============================" << std::endl;
    fuflog << "Stream Width  : " << width << "" << std::endl;
    fuflog << "Stream Height : " << height << "" << std::endl;
    fuflog << "Stream FPS    : " << depth_frames / 10 << "" << std::endl;
    fuflog << "Total frames  : " << depth_frames << "" << std::endl;
    fuflog << "Server log    :" << std::endl << server_log;
    fuflog << "============================" << std::endl;

    REQUIRE(depth_frames > 0);
}


