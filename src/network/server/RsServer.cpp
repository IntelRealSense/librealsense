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

    // Return version info
    svr.Get("/version", [&](const httplib::Request &, httplib::Response &res) {
        std::stringstream version;
        version << "LRS-Net version: ";
        version << (uint32_t)(RS2_NET_MAJOR_VERSION) << ".";
        version << (uint32_t)(RS2_NET_MINOR_VERSION) << ".";
        version << (uint32_t)(RS2_NET_PATCH_VERSION) << "\r\n";
        res.set_content(version.str(), "text/plain");
    });

    // Return device info
    svr.Get("/devinfo", [&](const httplib::Request &, httplib::Response &res) {
        std::stringstream devinfo;
        for (int32_t i = 0; i < static_cast<int>(RS2_CAMERA_INFO_COUNT); i++) {
            rs2_camera_info info_type = static_cast<rs2_camera_info>(i);
            devinfo << i << "|" << info_type << "|";
            if (m_dev.supports(info_type)) {
                devinfo << m_dev.get_info(info_type);
            } else {
                devinfo << "n/a";
            }
            devinfo << "\r\n";
        }
        res.set_content(devinfo.str(), "text/plain");
    });

    // Return sensors their intrinsics and sensors
    svr.Get("/query", [&](const httplib::Request &, httplib::Response &res) {
        res.set_content(m_sensors_desc, "text/plain");
    });

    // Return extrinsics
    svr.Get("/extrinsics", [&](const httplib::Request &, httplib::Response &res) {
        res.set_content(m_extrinsics.str(), "text/plain");
    });

    // Options 
    std::mutex options_mutex;    

    // Return options
    svr.Get("/options", [&](const httplib::Request &, httplib::Response &res) {
        std::stringstream options;

        options_mutex.lock();
        for (rs2::sensor sensor : m_dev.query_sensors()) {
            std::string sensor_name(sensor.supports(RS2_CAMERA_INFO_NAME) ? sensor.get_info(RS2_CAMERA_INFO_NAME) : "Unknown");
            // Prepare options list
            options << sensor_name;
            for (int32_t i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
                rs2_option option_type = static_cast<rs2_option>(i);
                options << "|" << std::to_string(i) << ","; // option index
                if (sensor.supports(option_type)) {
                    // Get the current value of the option
                    float current_value = sensor.get_option(option_type);
                    options << current_value << ",";
                    struct rs2::option_range current_range = sensor.get_option_range(option_type);
                    options << current_range.min << ",";
                    options << current_range.max << ",";
                    options << current_range.def << ",";
                    options << current_range.step;
                } else {
                    options << "n/a";
                }
            }
            options << "\r\n";
        }
        res.set_content(options.str(), "text/plain");
        options_mutex.unlock();
    });

    // Set options
    svr.Post("/options",
        [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
            if (req.is_multipart_form_data()) {
                std::cout << "No support for multipart messages" << std::endl;
            } else {
                std::string options;
                content_reader([&](const char *data, size_t data_length) {
                    options.append(data, data_length);
                    return true;
                });
                res.set_content(options, "text/plain");

                // Parse the request, it is in form
                // <sensor_name>|<opt1_index>,[<opt1_value>|n/a]|...|<optN_index>,[<optN_value>|n/a]
                while (!options.empty()) {
                    // get the sensor line
                    uint32_t line_pos = options.find("\r\n");
                    std::string sensor = options.substr(0, line_pos) + "|";
                    options.erase(0, line_pos + 2);

                    // get the sensor name
                    uint32_t pos = sensor.find("|");
                    std::string sensor_name = sensor.substr(0, pos);
                    sensor.erase(0, pos + 1);

                    // locate the proper sensor
                    auto sensors = m_dev.query_sensors();
                    auto s = sensors.begin();
                    for (; s != sensors.end(); s++) {
                        std::string sname(s->supports(RS2_CAMERA_INFO_NAME) ? s->get_info(RS2_CAMERA_INFO_NAME) : "Unknown");
                        if (std::strcmp(sname.c_str(), sensor_name.c_str()) == 0) {
                            break;
                        }
                    }

                    // locate the option and update is changed
                    if (s != sensors.end()) {
                        while (!sensor.empty()) {
                            pos = sensor.find(",");
                            uint32_t idx = std::stoul(sensor.substr(0, pos).c_str());
                            sensor.erase(0, pos + 1);
                            pos = sensor.find("|");
                            std::string vals = sensor.substr(0, pos + 1);
                            sensor.erase(0, pos + 1);
                            if (std::strcmp(vals.c_str(), "n/a|") != 0) {
                                pos = vals.find("|");
                                float val = std::stof(vals.substr(0, pos).c_str());

                                options_mutex.lock();
                                if (s->supports((rs2_option)idx))
                                if (s->get_option((rs2_option)idx) != val) {
                                    // std::cout << "Setting option " << idx << " to " << val << std::endl;
                                    try {
                                        s->set_option((rs2_option)idx, val);
                                    } catch(const rs2::error& e) {
                                        std::cout << "Failed to set option " << idx << ". (" << e.what() << ")" << std::endl;
                                    }
                                }
                                options_mutex.unlock();
                            }
                        }
                    } else {
                        std::cout << "Unknown sensor specified: " << sensor_name << std::endl;
                    }
                }
            }
        }
    );
  
    svr.listen("0.0.0.0", 8080);
}

server::server(rs2::device dev, std::string addr, int port) : m_dev(dev)
{
    ReceivingInterfaceAddr = inet_addr(addr.c_str());

    // Begin by setting up our usage environment:
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    srv = RTSPServer::createNew(*env, 8554, NULL);
    if (srv == NULL) {
        std::cout << "Failed to create RTSP server: " << env->getResultMsg() << std::endl;
        exit(1);
    }

    std::map<StreamIndex, rs2::stream_profile> unique_streams;
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

        std::string sensor_path = sensor_name;
        ServerMediaSession* sms = ServerMediaSession::createNew(*env, sensor_path.c_str(), sensor_name.c_str(), "Session streamed by LRS-Net");

        // Prepare profiles
        std::stringstream profile_keys;
        for (auto profile : sensor.get_stream_profiles()) {
            std::cout <<  "Profile : " << slib::print_profile(profile);

            if (profile.format() == RS2_FORMAT_YUYV || 
                profile.format() == RS2_FORMAT_UYVY || 
                profile.format() == RS2_FORMAT_Z16  ||
                profile.format() == RS2_FORMAT_Y8   ||
                profile.format() == RS2_FORMAT_MOTION_XYZ32F) 
            {
                // Count unique streams and save one profile of each kind to calculate extrinsics later
                unique_streams[StreamIndex(profile.stream_type(), profile.stream_index())] = profile;

                sms->addSubsession(RsServerMediaSubsession::createNew(*env, pfq, profile));
                std::cout << " - ACCEPTED" << std::endl;
                profile_keys << "|" << slib::profile2key(profile) << ",";

                // add intrinsics
                if (profile.is<rs2::video_stream_profile>()) {
                    rs2_intrinsics intrinsics = profile.as<rs2::video_stream_profile>().get_intrinsics();
                    profile_keys << intrinsics.width << "," << intrinsics.height << ",";
                    profile_keys << intrinsics.ppx   << "," << intrinsics.ppy    << ",";
                    profile_keys << intrinsics.fx    << "," << intrinsics.fy     << ",";
                    for (int c = 0; c < 5; c++) {
                        profile_keys << intrinsics.coeffs[c] << ",";
                    }
                    profile_keys << (int32_t)(intrinsics.model);
                } else {
                    profile_keys << "n/a";
                }
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

    // Prepare extrinsics
    for (auto from_stream : unique_streams) {
        std::stringstream from_name;
        from_name << from_stream.first.first;
        if (from_stream.first.second) {
            from_name << " " << from_stream.first.second;
        }
        auto from_profile = from_stream.second;
        for (auto to_stream : unique_streams) {
            std::stringstream to_name;
            to_name << to_stream.first.first;
            if (to_stream.first.second) {
                to_name << " " << to_stream.first.second;
            }
            auto to_profile = to_stream.second;

            m_extrinsics << from_name.str() << "|" << to_name.str() << "|";
            m_extrinsics << slib::profile2key(from_profile) << "|" << slib::profile2key(to_profile);

            try {
                // Given two streams, use the get_extrinsics_to() function to get the transformation from the stream to the other stream
                rs2_extrinsics extrinsics = from_profile.get_extrinsics_to(to_profile);
                // std::cout << "From " << from_name << " to " << to_name << std::endl;
                for (int t = 0; t < 3; t++) {
                    m_extrinsics << "|" << extrinsics.translation[t];
                }
                for (int r = 0; r < 9; r++) {
                    m_extrinsics << "|" << extrinsics.rotation[r];
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to get extrinsics for the streams (" << from_name.str() << " => " << to_name.str() << "): " << e.what() << std::endl;
            }

            m_extrinsics << "\r\n";
        }
    }

    m_httpd = std::thread( [this](){ doHTTP(); } ); 
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

