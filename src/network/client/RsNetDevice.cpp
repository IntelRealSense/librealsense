// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "RsNetDevice.h"

#include <api.h>
#include <librealsense2-net/rs_net.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <functional>

#include <stdlib.h>
#include <math.h>

#include <httplib.h>

using namespace std::placeholders;

rs_net_device::rs_net_device(rs2::software_device sw_device, std::string ip_address)
    : m_device(sw_device), m_running(true)
{
    // parse the parameters and set address and port
    int colon = ip_address.find(":");
    m_ip_address = ip_address.substr(0, colon); // 10.10.10.10:8554 => 10.10.10.10
    m_ip_port = 8554; // default RTSP port
    if (colon != -1) {
        try {
            m_ip_port = std::stoi(ip_address.substr(colon + 1)); // 10.10.10.10:8554 => 8554
        } catch(...) { 
            // ignore wrong port and use default
            LOG_WARNING("WARNING: invalid port specified, using default.");
        }
    }
    uint32_t pos = 0;

    // Obtain device information via HTTP and set it for the software device. 
    httplib::Client client(m_ip_address, 8080);
    httplib::Result ver = client.Get("/version");
    if (ver) {
        if (ver->status == 200) {
            // parse the response in form:
            // <LRS-Net version: X.Y.Z>
            std::string version;
            version += "LRS-Net version: ";
            version += std::to_string(RS2_NET_MAJOR_VERSION) + ".";
            version += std::to_string(RS2_NET_MINOR_VERSION) + ".";
            version += std::to_string(RS2_NET_PATCH_VERSION) + "\r\n";

            std::string server_version = ver->body;
            if (std::strcmp(version.c_str(), server_version.c_str())) {
                throw std::runtime_error("Version mismatch.");
            }
        } else {
            throw std::runtime_error("Error in server response: version.");
        }
    } else {
        throw std::runtime_error("Error connecting to the server.");
    }

    httplib::Result inf = client.Get("/devinfo");
    if (inf) {
        if (inf->status == 200) {
            // parse the response in form:
            // <info_index|info_desc|info_val>

            std::string devinfo = inf->body;
            while (!devinfo.empty()) {
                // get the index
                pos = devinfo.find("|");
                uint32_t idx = std::stoul(devinfo.substr(0, pos).c_str());
                devinfo.erase(0, pos + 1);

                // get the description
                pos = devinfo.find("|");
                std::string desc = devinfo.substr(0, pos);
                devinfo.erase(0, pos + 1);

                pos = devinfo.find("\r\n");
                std::string val = devinfo.substr(0, pos);
                devinfo.erase(0, pos + 2);
                if (std::strcmp(val.c_str(), "n/a") != 0) {
                    std::stringstream ss_devinfo;
                    ss_devinfo << std::left << std::setw(30) << desc << " : " << val;
                    LOG_DEBUG(ss_devinfo.str());
                    m_device.register_info((rs2_camera_info)idx, val);
                }
            }
        } else {
            throw std::runtime_error("Error in server response: device information.");
        }
    } else {
        throw std::runtime_error("Error obtaining device information.");
    }

    // Check the recovery mode
    if (m_device.supports(RS2_CAMERA_INFO_NAME) && (std::string(m_device.get_info(RS2_CAMERA_INFO_NAME)).find("Recovery") != std::string::npos)) {
        throw std::runtime_error("Network device is in the recovery mode.");
    }

    // Obtain number of sensors and their names via HTTP and construct the software device. 
    httplib::Result res = client.Get("/query");
    if (res) {
        if (res->status == 200) {
            // parse the response in form:
            // <sensor_name>|<sensor_mrl>|<sensor_profile1,intrinsics1>|<sensor_profile2,intrinsics2>|...|<sensor_profileN,intrinsicsN>
            // intrinsics = [width,heigth,ppx,ppy,fx,fy,coeff[0],coeff[1],coeff[2],coeff[3],coeff[4],model|n/a]

            std::string query = res->body;
            while (!query.empty()) {
                // get the sensor line
                uint32_t line_pos = query.find("\r\n");
                std::string sensor = query.substr(0, line_pos) + "|";
                query.erase(0, line_pos + 2);

                // get the sensor name
                pos = sensor.find("|");
                std::string sensor_name = sensor.substr(0, pos);
                sensor.erase(0, pos + 1);

                // TODO: move the following code to the NetSensor's constructor
                NetSensor netsensor(new rs_net_sensor(std::make_shared<rs2::software_sensor>(m_device.add_sensor(sensor_name)), sensor_name));

                // get the sensor MRL
                pos = sensor.find("|");
                netsensor->set_mrl(sensor.substr(0, pos));
                sensor.erase(0, pos + 1);

                while (!sensor.empty()) {
                    pos = sensor.find(",");
                    uint64_t key = std::stoull(sensor.substr(0, pos).c_str());
                    sensor.erase(0, pos + 1);

                    // get intrinsics
                    rs2_intrinsics intrinsics_val = {0};
                    pos = sensor.find("|");
                    std::string intrinsics_str = sensor.substr(0, pos);
                    sensor.erase(0, pos + 1);
                    if (std::strcmp(intrinsics_str.c_str(), "n/a") != 0) {
                        pos = intrinsics_str.find(",");
                        intrinsics_val.width = std::stoi(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);

                        pos = intrinsics_str.find(",");
                        intrinsics_val.height = std::stoi(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);

                        pos = intrinsics_str.find(",");
                        intrinsics_val.ppx = std::stof(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);

                        pos = intrinsics_str.find(",");
                        intrinsics_val.ppy = std::stof(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);

                        pos = intrinsics_str.find(",");
                        intrinsics_val.fx = std::stof(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);

                        pos = intrinsics_str.find(",");
                        intrinsics_val.fy = std::stof(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);

                        for (int c = 0; c < 5; c++) {
                            pos = intrinsics_str.find(",");
                            intrinsics_val.coeffs[c] = std::stof(intrinsics_str.substr(0, pos).c_str());
                            intrinsics_str.erase(0, pos + 1);
                        }

                        pos = intrinsics_str.find("|");
                        intrinsics_val.model = (rs2_distortion)std::stoi(intrinsics_str.substr(0, pos).c_str());
                        intrinsics_str.erase(0, pos + 1);
                    }

                    netsensor->add_profile(key, intrinsics_val);
                }

                sensors.emplace_back(netsensor);
            }
        } else {
            throw std::runtime_error("Error in server response: profiles.");
        }
    } else {
        throw std::runtime_error("Error obtaining device profiles.");
    }

    // Obtain sensors options via HTTP and update the software device. 
    httplib::Result opt = client.Get("/options");
    if (opt) {
        if (opt->status == 200) {
            // parse the response in form:
            // <sensor_name>|<opt1_index>,[n/a|<opt1_value>,<opt1_min>,<opt1_max>,<opt1_def>,<opt1_step>]|...|<optN_index>,[n/a|<optN_value>,<optN_min>,<optN_max>,<optN_def>,<optN_step>]>

            std::string query = opt->body;
            while (!query.empty()) {
                // get the sensor line
                uint32_t line_pos = query.find("\r\n");
                std::string sensor = query.substr(0, line_pos) + "|";
                query.erase(0, line_pos + 2);

                // get the sensor name
                uint32_t pos = sensor.find("|");
                std::string sensor_name = sensor.substr(0, pos);
                sensor.erase(0, pos + 1);

                // locate the proper NetSensor
                auto s = sensors.begin();
                for (; s != sensors.end(); s++) {
                    if (std::strcmp((*s)->get_name().c_str(), sensor_name.c_str()) == 0) {
                        break;
                    }
                }

                while (!sensor.empty()) {
                    pos = sensor.find(",");
                    uint32_t idx = std::stoul(sensor.substr(0, pos).c_str());
                    sensor.erase(0, pos + 1);
                    pos = sensor.find("|");
                    std::string vals = sensor.substr(0, pos + 1);
                    sensor.erase(0, pos + 1);
                    if (std::strcmp(vals.c_str(), "n/a|") != 0) {
                        float val = 0; 
                        rs2::option_range range = {0};

                        pos = vals.find(",");
                        val = std::stof(vals.substr(0, pos).c_str());
                        vals.erase(0, pos + 1);

                        pos = vals.find(",");
                        range.min = std::stof(vals.substr(0, pos).c_str());
                        vals.erase(0, pos + 1);

                        pos = vals.find(",");
                        range.max = std::stof(vals.substr(0, pos).c_str());
                        vals.erase(0, pos + 1);

                        pos = vals.find(",");
                        range.def = std::stof(vals.substr(0, pos).c_str());
                        vals.erase(0, pos + 1);

                        pos = vals.find("|");
                        range.step = std::stof(vals.substr(0, pos).c_str());
                        vals.erase(0, pos + 1);

                        (*s)->add_option(idx, val, range);
                    }
                }
            }
        } else {
            throw std::runtime_error("Error in server response: options.");
        }
    } else {
        throw std::runtime_error("Error obtaining device options.");
    }

    LOG_INFO("Software device is ready.");

    m_options = std::thread( [this](){ doOptions(); } ); 

    for (auto netsensor : sensors) netsensor->start();

    // m_extrinsics = std::thread( [this]() { doExtrinsics(); });
    doExtrinsics();
}

rs_net_device::~rs_net_device() {
    m_running = false;
    // if (m_extrinsics.joinable()) m_extrinsics.join();
    if (m_options.joinable()) m_options.join();
}


void rs_net_device::doExtrinsics() {
    LOG_INFO("Extrinsics initialization thread started.");

    // Prepare extrinsics map
    httplib::Client client(m_ip_address, 8080);
    httplib::Result ext = client.Get("/extrinsics");
    if (ext) {
        if (ext->status == 200) {
            // parse the response in form:
            // <sensor_name_from>|<sensor_name_to>|<profie_name_from>|<profie_name_to>|<translation1>,...,<translation3>|<rotation1>,...,<rotation9>

            std::string query = ext->body;
            while (!query.empty()) {
                // get the one extrinsics line
                uint32_t line_pos = query.find("\r\n");
                std::string extrinsics = query.substr(0, line_pos);
                query.erase(0, line_pos + 2);

                // get the first stream name
                uint32_t pos = extrinsics.find("|");
                std::string from_name = extrinsics.substr(0, pos);
                extrinsics.erase(0, pos + 1);

                // get the second stream name
                pos = extrinsics.find("|");
                std::string to_name = extrinsics.substr(0, pos);
                extrinsics.erase(0, pos + 1);

                // get the first stream profile
                pos = extrinsics.find("|");
                uint64_t from_profile = std::stoll(extrinsics.substr(0, pos));
                extrinsics.erase(0, pos + 1);
                rs2_video_stream from_stream = slib::key2stream(from_profile);

                // get the second stream profile
                pos = extrinsics.find("|");
                uint64_t to_profile = std::stoll(extrinsics.substr(0, pos));
                extrinsics.erase(0, pos + 1);
                rs2_video_stream to_stream = slib::key2stream(to_profile);

                rs2_extrinsics extr;

                // translation
                for (int t = 0; t < 3; t++) {
                    pos = extrinsics.find(",");
                    extr.translation[t] = std::stof(extrinsics.substr(0, pos).c_str());
                    extrinsics.erase(0, pos + 1);
                }

                // rotation
                for (int r = 0; r < 9; r++) {
                    pos = extrinsics.find(",");
                    extr.rotation[r] = std::stof(extrinsics.substr(0, pos).c_str());
                    extrinsics.erase(0, pos + 1);
                }

                // update the extrinsincs map
                m_extrinsics_map[StreamPair(StreamIndex(from_stream.type, from_stream.index), StreamIndex(to_stream.type, to_stream.index))] = extr;
            }
        } else {
            throw std::runtime_error("Error in server response: extrinsics.");
        }
    } else {
        throw std::runtime_error("Error obtaining device extrinsics.");
    }

    // gather the list of all profiles
    std::vector<rs2::stream_profile> profiles;
    for (rs2::sensor sensor : m_device.query_sensors()) {
        auto sprofiles = sensor.get_stream_profiles();
        profiles.insert(profiles.end(), sprofiles.begin(), sprofiles.end());
    }

    // update the extrinsics for all profiles
    // WARNING: takes a long time
    for (auto from_profile : profiles) {
        for (auto to_profile : profiles) {
            from_profile.register_extrinsics_to(to_profile, 
                m_extrinsics_map[StreamPair(StreamIndex(from_profile.stream_type(), from_profile.stream_index()), StreamIndex(to_profile.stream_type(), to_profile.stream_index()))]);
        }
    }

    LOG_INFO("Extrinsics initialization thread exited.");
}

void rs_net_device::doOptions() {
    LOG_INFO("Options synchronization thread started.");

    std::string options_prev;

    while (m_running) {
        std::string options;
        for (auto netsensor : sensors) {
            options += netsensor->get_name();

            for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
                options += "|";

                rs2_option option_type = static_cast<rs2_option>(i);

                options += std::to_string(i); // option index
                options += ",";

                if (netsensor->get_sensor()->supports(option_type) && !netsensor->get_sensor()->is_option_read_only(option_type)) {
                    // Get the current value of the option
                    float current_value = netsensor->get_sensor()->get_option(option_type);
                    options += std::to_string(current_value);
                } else {
                    options += "n/a";
                }
            }
            options += "\r\n";
        }

        if (std::strcmp(options.c_str(), options_prev.c_str())) {
            httplib::Client client(m_ip_address, 8080);
            client.Post("/options", options.c_str(), "text/plain");
            options_prev = options;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
    }

    LOG_INFO("Options synchronization thread exited.");
}
