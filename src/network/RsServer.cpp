// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#include <httplib.h>

#include <signal.h>

#include <iostream>
#include <queue>
#include <thread>
#include <functional>

#include <liveMedia.hh>
#include <GroupsockHelper.hh>
#include "BasicUsageEnvironment.hh"

#include <librealsense2/rs.hpp>

#include "RsServer.h"
#include "RsSensor.h"
#include "RsServerMediaSubsession.h"

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

using namespace TCLAP;

void server::doHTTP() {
    std::cout << "Internal HTTP server started." << std::endl;

    httplib::Server svr;

    svr.Get("/query", [&](const httplib::Request &, httplib::Response &res) {
        res.set_content(m_sensors_desc, "text/plain");
    });

    svr.listen("0.0.0.0", 8080);
}

server::server(rs2::device dev, std::string addr, int port)
{
    m_httpd  = std::thread( [this](){ doHTTP(); } ); 

    m_serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    m_name   = dev.get_info(RS2_CAMERA_INFO_NAME);

    ReceivingInterfaceAddr = inet_addr(addr.c_str());
    OutPacketBuffer::increaseMaxSizeTo(640*480*2); // TODO: put real values

    // Begin by setting up our usage environment:
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    // srv = RTSPServer::createNew(*env, 8554, NULL);
    srv = RsRTSPServer::createNew(*env, 8554);
    if (srv == NULL) {
        std::cout << "Failed to create RTSP server: " << env->getResultMsg() << std::endl;
        exit(1);
    }

    for (rs2::sensor sensor : dev.query_sensors()) {
        frames_queue* pfq = new frames_queue(sensor);

        std::string sensor_name(sensor.supports(RS2_CAMERA_INFO_NAME) ? sensor.get_info(RS2_CAMERA_INFO_NAME) : "Unknown");

        std::cout << "Sensor\t: " << sensor_name.c_str();
        if (sensor.get_active_streams().size() > 0) {
            std::cout << " is streaming, stopping and closing it.";
            sensor.stop();
            sensor.close();            
        }
        std::cout << std::endl;

        // std::string sensor_path = m_serial + "/" + sensor_name;
        std::string sensor_path = sensor_name;
        ServerMediaSession* sms = ServerMediaSession::createNew(*env, sensor_path.c_str(), sensor_name.c_str(), "Session streamed by LRS-Net");

        std::stringstream profile_keys;
        for (auto profile : sensor.get_stream_profiles()) {
            std::cout <<  "Profile : " << slib::print_profile(profile);

            if (profile.format() == RS2_FORMAT_YUYV || 
                profile.format() == RS2_FORMAT_UYVY || 
                profile.format() == RS2_FORMAT_Z16  ||
                profile.format() == RS2_FORMAT_Y8   ||
                profile.format() == RS2_FORMAT_MOTION_XYZ32F) 
            {
                sms->addSubsession(RsServerMediaSubsession::createNew(*env, pfq, profile));
                std::cout << " - ACCEPTED" << std::endl;
                profile_keys << "|" << slib::profile2key(profile);
                continue;
            }
            std::cout << " - ignored" << std::endl;
        }

        srv->addServerMediaSession(sms);
        char* url = srv->rtspURL(sms); // should be deallocated later
        std::cout << "Access\t: " << url << std::endl << std::endl;

        if (profile_keys.str().size()) m_sensors_desc += sensor_name + "|" + url + profile_keys.str() + "\r\n";

        delete[] url;
    }
    // std::cout << m_sensors_desc;
}

void server::start()
{
    env->taskScheduler().doEventLoop(); // does not return
}

void server::stop()
{
}

server::~server()
{
    Medium::close(srv);
    env->reclaim();
    env = NULL;
    delete scheduler;
    scheduler = NULL;
}

