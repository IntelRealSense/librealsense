// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <httplib.h>

#include <signal.h>

#include <iostream>
#include <queue>
#include <thread>
#include <functional>
#include <fstream>

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

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>

/* mkstemp extracted from libc/sysdeps/posix/tempname.c.  Copyright
   (C) 1991-1999, 2000, 2001, 2006 Free Software Foundation, Inc.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.  */

static const char letters[] =
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

/* Generate a temporary file name based on TMPL.  TMPL must match the
   rules for mk[s]temp (i.e. end in "XXXXXX").  The name constructed
   does not exist at the time of the call to mkstemp.  TMPL is
   overwritten with the result.  */
int
mkstemp(char *tmpl)
{
    int len;
    char *XXXXXX;
    static unsigned long long value;
    unsigned long long random_time_bits;
    unsigned int count;
    int fd = -1;
    int save_errno = errno;

    /* A lower bound on the number of temporary files to attempt to
       generate.  The maximum total number of temporary file names that
       can exist for a given template is 62**6.  It should never be
       necessary to try all these combinations.  Instead if a reasonable
       number of names is tried (we define reasonable as 62**3) fail to
       give the system administrator the chance to remove the problems.  */
#define ATTEMPTS_MIN (62 * 62 * 62)

       /* The number of times to attempt to generate a temporary file.  To
          conform to POSIX, this must be no smaller than TMP_MAX.  */
#if ATTEMPTS_MIN < TMP_MAX
    unsigned int attempts = TMP_MAX;
#else
    unsigned int attempts = ATTEMPTS_MIN;
#endif

    len = strlen(tmpl);
    if (len < 6 || strcmp(&tmpl[len - 6], "XXXXXX"))
    {
        errno = EINVAL;
        return -1;
    }

    /* This is where the Xs start.  */
    XXXXXX = &tmpl[len - 6];

    /* Get some more or less random data.  */
    {
        SYSTEMTIME      stNow;
        FILETIME ftNow;

        // get system time
        GetSystemTime(&stNow);
        stNow.wMilliseconds = 500;
        if (!SystemTimeToFileTime(&stNow, &ftNow))
        {
            errno = -1;
            return -1;
        }

        random_time_bits = (((unsigned long long)ftNow.dwHighDateTime << 32)
            | (unsigned long long)ftNow.dwLowDateTime);
    }
    value += random_time_bits ^ (unsigned long long)GetCurrentThreadId();

    for (count = 0; count < attempts; value += 7777, ++count)
    {
        unsigned long long v = value;

        /* Fill in the random bits.  */
        XXXXXX[0] = letters[v % 62];
        v /= 62;
        XXXXXX[1] = letters[v % 62];
        v /= 62;
        XXXXXX[2] = letters[v % 62];
        v /= 62;
        XXXXXX[3] = letters[v % 62];
        v /= 62;
        XXXXXX[4] = letters[v % 62];
        v /= 62;
        XXXXXX[5] = letters[v % 62];

        fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, _S_IREAD | _S_IWRITE);
        if (fd >= 0)
        {
            errno = save_errno;
            return fd;
        }
        else if (errno != EEXIST)
            return -1;
    }

    /* We got out of the loop because we ran out of combinations to try.  */
    errno = EEXIST;
    return -1;
}
#endif

void server::doSW_Upgrade() {
    // First, shutdown the RTSP server
    m_progress = "shutting down the RTSP server";
    srv->close(srv->envir(), srv->name());
    std::this_thread::sleep_for(std::chrono::seconds(1));

    char fname[] = "/tmp/XXXXXX";
    int file = mkstemp(fname);
    write(file, m_package.c_str(), m_package.size());
    close(file);

    LOG_INFO("Received the package " << fname << " of " << std::dec << m_package.size() << " bytes to perform the software upgrade. ");

    std::stringstream cmd;
    cmd << "/usr/bin/systemd-run --on-active=1 /usr/bin/dpkg -i " << fname;
    system(cmd.str().c_str());
}

void server::doFW_Upgrade() {
    // First, shutdown the RTSP server
    m_progress = "shutting down the RTSP server";
    srv->close(srv->envir(), srv->name());
    std::this_thread::sleep_for(std::chrono::seconds(1));

    LOG_INFO("Received the image of " << std::dec << m_image.size() << " bytes to perform the firmware upgrade.");

    #if 0
    if (backup_arg.isSet()) {
        std::cout << std::endl << "backing-up device flash: " << std::endl;

        auto flash = d.as<rs2::updatable>().create_flash_backup([&](const float progress)
        {
            printf("\rflash backup progress: %d[%%]", (int)(progress * 100));
        });

        auto temp = backup_arg.getValue();
        std::ofstream file(temp.c_str(), std::ios::binary);
        file.write((const char*)flash.data(), flash.size());
    }
    #endif

    std::vector<uint8_t> fw_image(m_image.begin(), m_image.end()); 
    m_progress = "Starting to update the device";
    LOG_INFO(m_progress);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    rs2::context ctx;
    rs2::update_device new_fw_update_device;
    std::condition_variable cv;
    std::mutex mutex;

    if (m_dev.is<rs2::updatable>()) {
        // Update device
        ctx.set_devices_changed_callback([&](rs2::event_information& info) {
            if (info.get_new_devices().size() == 0) return;

            for (auto&& d : info.get_new_devices()) {
                std::lock_guard<std::mutex> lk(mutex);
                if (d.is<rs2::update_device>())
                    new_fw_update_device = d;
            }

            if (new_fw_update_device) cv.notify_one();
        });

        m_dev.as<rs2::updatable>().enter_update_state();
        std::unique_lock<std::mutex> lk(mutex);
        if (!cv.wait_for(lk, std::chrono::seconds( /* WAIT_FOR_DEVICE_TIMEOUT */ 10), [&] { return new_fw_update_device; })) {
            LOG_ERROR("Failed to locate a device in firmware update mode");
            exit(EXIT_FAILURE);
        }
    } else {
        new_fw_update_device = m_dev;
    }

    m_progress = "Firmware update started";
    LOG_INFO(m_progress);
    try {
        new_fw_update_device.update(fw_image, [&](const float progress) {
            std::stringstream ss;
            ss << "Firmware update progress: " << (int)(progress * 100) << "%";
            m_progress = ss.str();
            std::cout << m_progress << "\r";
        });
        m_progress = "Firmware update is done";
        std::cout << std::endl;
        LOG_INFO(m_progress);
    } catch (const std::exception& e) {
        m_progress = e.what();
        LOG_ERROR(std::endl << "ERROR: " << m_progress << std::endl);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        exit(EXIT_FAILURE);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(EXIT_SUCCESS);
}

void server::doHTTP() {
    LOG_INFO("Internal HTTP server started.");

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
                LOG_ERROR("No support for multipart messages");
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
                                    LOG_DEBUG("Setting option " << idx << " to " << val);
                                    try {
                                        s->set_option((rs2_option)idx, val);
                                    } catch(const rs2::error& e) {
                                        LOG_WARNING("Failed to set option " << idx << ". (" << e.what() << ")");
                                    }
                                }
                                options_mutex.unlock();
                            }
                        }
                    } else {
                        LOG_ERROR("Unknown sensor specified: " << sensor_name);
                    }
                }
            }
        }
    );

    // SW upgrade
    // TODO: Add CRC validation
    bool ongoing = false;
    svr.Post("/sw_upgrade",
        [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
            if (req.is_multipart_form_data()) {
                LOG_ERROR("No support for multipart messages");
            } else {
                if (ongoing) {
                    return;
                } else {
                    ongoing = true;

                    content_reader([&](const char *data, size_t data_length) {
                        m_package.append(data, data_length);
                        return true;
                    });
                    res.set_content(m_package, "application/octet-stream");

                    m_sw_upgrade = std::thread( [this](){ doSW_Upgrade(); } ); 
                }
            }
        }
    );

    // FW upgrade
    // TODO:;Add CRC validation
    svr.Post("/fw_upgrade",
        [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
            if (req.is_multipart_form_data()) {
                LOG_ERROR("No support for multipart messages");
            } else {
                m_progress = "Started";
                m_image.clear();
                content_reader([&](const char *data, size_t data_length) {
                    m_image.append(data, data_length);
                    return true;
                });
                res.set_content(m_image, "application/octet-stream");

                m_fw_upgrade = std::thread( [this](){ doFW_Upgrade(); } ); 
            }
        }
    );

    // Return extrinsics
    svr.Get("/fw_upgrade_status", [&](const httplib::Request &, httplib::Response &res) {
        res.set_content(m_progress, "text/plain");
    });

    svr.listen("0.0.0.0", 8080);
}

server::server(rs2::device dev, std::string addr, int port) : m_dev(dev), m_progress("not active")
{
    ReceivingInterfaceAddr = inet_addr(addr.c_str());

    // Begin by setting up our usage environment:
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    srv = RTSPServer::createNew(*env, 8554, NULL);
    if (srv == NULL) {
        LOG_FATAL("Failed to create RTSP server: " << env->getResultMsg());
        exit(1);
    }

    std::map<StreamIndex, rs2::stream_profile> unique_streams;
    for (rs2::sensor sensor : dev.query_sensors()) {
        frames_queue* pfq = new frames_queue(sensor);

        std::string sensor_name(sensor.supports(RS2_CAMERA_INFO_NAME) ? sensor.get_info(RS2_CAMERA_INFO_NAME) : "Unknown");

        std::stringstream ss_sensor;
        ss_sensor << "Sensor\t: " << sensor_name.c_str();
        if (sensor.get_active_streams().size() > 0) {
            ss_sensor << " is streaming, stopping and closing it.";
            sensor.stop();
            sensor.close();            
        }
        LOG_INFO(ss_sensor.str());

        std::string sensor_path = sensor_name;
        ServerMediaSession* sms = ServerMediaSession::createNew(*env, sensor_path.c_str(), sensor_name.c_str(), "Session streamed by LRS-Net");

        // Prepare profiles
        std::stringstream profile_keys;
        for (auto profile : sensor.get_stream_profiles()) {
            std::stringstream ss_profile;
            ss_profile << "Profile : " << slib::print_profile(profile);

            if (profile.format() == RS2_FORMAT_YUYV || 
                profile.format() == RS2_FORMAT_UYVY || 
                profile.format() == RS2_FORMAT_Z16  ||
                profile.format() == RS2_FORMAT_Y8   ||
                profile.format() == RS2_FORMAT_RAW8 ||
                profile.format() == RS2_FORMAT_MOTION_XYZ32F) 
            {
                // Count unique streams and save one profile of each kind to calculate extrinsics later
                unique_streams[StreamIndex(profile.stream_type(), profile.stream_index())] = profile;

                sms->addSubsession(RsServerMediaSubsession::createNew(*env, pfq, profile));
                ss_profile << " ACCEPTED";
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
                // continue;
            } else {
                ss_profile << " ignored";
            }
            LOG_INFO(ss_profile.str());
        }

        srv->addServerMediaSession(sms);
        char* url = srv->rtspURL(sms); // should be deallocated later
        LOG_INFO("Access\t: " << url << std::endl);

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
                LOG_DEBUG("From " << from_name.str() << " to " << to_name.str());
                for (int t = 0; t < 3; t++) {
                    m_extrinsics << "|" << extrinsics.translation[t];
                }
                for (int r = 0; r < 9; r++) {
                    m_extrinsics << "|" << extrinsics.rotation[r];
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to get extrinsics for the streams (" << from_name.str() << " => " << to_name.str() << "): " << e.what());
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

