// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <csignal>
#include <functional>

#include <librealsense2/rs.hpp>
#include <librealsense2-net/rs_net.hpp>

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

#define DBG CLOG(DEBUG,   "librealsense")
#define ERR CLOG(ERROR,   "librealsense")
#define WRN CLOG(WARNING, "librealsense")
#define INF CLOG(INFO,    "librealsense")

// Signal handler
namespace {
    std::function<void(int)> shutdown_handler;
    void signal_handler(int signal) { shutdown_handler(signal); }
}

int main(int argc, char** argv)
{
    // Log engine initialization and configuration
#ifdef BUILD_SHARED_LIBS
    // Configure the logger
    el::Configurations conf;
    conf.set(el::Level::Global, el::ConfigurationType::Format, "[%level] %msg");
    conf.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "true");

    conf.set(el::Level::Trace,   el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Debug,   el::ConfigurationType::Enabled, "false");
    conf.set(el::Level::Fatal,   el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Error,   el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Warning, el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Verbose, el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Info,    el::ConfigurationType::Enabled, "true");
    conf.set(el::Level::Unknown, el::ConfigurationType::Enabled, "true");

    el::Loggers::reconfigureLogger("librealsense", conf);
#else
    rs2::log_to_callback(RS2_LOG_SEVERITY_INFO,
        [&](rs2_log_severity severity, rs2::log_message const& msg)
    {
        std::cout << msg.raw() << "\n";
    });
#endif

    INF << "Rs-server is running\n";

    // Handle the command-line parameters
    TCLAP::CmdLine cmd("LRS Network Extentions Server", ' ', RS2_API_VERSION_STR);
    TCLAP::SwitchArg arg_enable_compression("c", "enable-compression", "Enable video compression");
    TCLAP::ValueArg<std::string> arg_address("i", "interface-address", "Address of the interface to bind on", false, "", "string");
    TCLAP::ValueArg<unsigned int> arg_port("p", "port", "RTSP port to listen on", false, 8554, "integer");

    cmd.add(arg_enable_compression);
    cmd.add(arg_address);
    cmd.add(arg_port);

    cmd.parse(argc, argv);

    std::string addr = "0.0.0.0";
    if (arg_address.isSet())
    {
        addr = arg_address.getValue();
    }

    int port = 8554;
    if (arg_port.isSet())
    {
        port = arg_port.getValue();
    }

    // Get the device
    rs2::context ctx;
    rs2::device_list devices = ctx.query_devices();
    rs2::device dev = devices[0];

    // Create the server using supplied parameters
    rs_server_params params = { strdup(addr.c_str()), port, 0 };
    rs2::net_server rs_server(dev, params);

    // Install the exit handler
    shutdown_handler = [&](int signal) {
        INF << "Exiting\n";
        rs_server.stop();
        exit(signal);
    };
    std::signal(SIGINT, signal_handler);

    // Start the server
    rs_server.start(); // Never returns

    return 0;
}