// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>

#include <liveMedia.hh>
#include <GroupsockHelper.hh>
#include <signal.h>
#include "RsUsageEnvironment.h"
#include "RsSource.hh"
#include "RsServerMediaSubsession.h"
#include "RsDevice.hh"
#include "RsRTSPServer.hh"
#include "RsServerMediaSession.h"
#include "RsCommon.h"

struct server
{
    rs2::device selected_device;
    RsRTSPServer* rtspServer;
    UsageEnvironment* env;
    std::shared_ptr<RsDevice> rsDevice;
    std::vector<rs2::video_stream_profile> supported_stream_profiles; // streams for extrinsics map creation
    std::vector<RsSensor> sensors;
    TaskScheduler* scheduler;

    void main(int argc, char** argv)
    {
        std::cout << "Rs-server is running\n";

        START_EASYLOGGINGPP(argc, argv);

        OutPacketBuffer::increaseMaxSizeTo(MAX_MESSAGE_SIZE);

        // Begin by setting up our usage environment:
        scheduler = BasicTaskScheduler::createNew();
        env = RSUsageEnvironment::createNew(*scheduler);

        rsDevice = std::make_shared<RsDevice>(env);
        rtspServer = RsRTSPServer::createNew(*env, rsDevice, 8554);

        if(rtspServer == NULL)
        {
            *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
            exit(1);
        }

        sensors = rsDevice.get()->getSensors();
        for(auto sensor : sensors)
        {
            RsServerMediaSession* sms;
            if(sensor.getSensorName().compare(STEREO_SENSOR_NAME) == 0 || sensor.getSensorName().compare(RGB_SENSOR_NAME) == 0)
            {
                sms = RsServerMediaSession::createNew(*env, sensor, sensor.getSensorName().data(), "", "Session streamed by \"realsense streamer\"", True);
            }
            else
            {
                break;
            }

            for(auto stream_profile : sensor.getStreamProfiles())
            {
                rs2::video_stream_profile stream = stream_profile.second;
                if(stream.format() == RS2_FORMAT_BGR8 || stream.format() == RS2_FORMAT_RGB8 || stream.format() == RS2_FORMAT_Z16 || stream.format() == RS2_FORMAT_Y8 || stream.format() == RS2_FORMAT_YUYV || stream.format() == RS2_FORMAT_UYVY)
                {
                    if(stream.fps() == 6)
                    {
                        if((stream.width() == 1280 && stream.height() == 720) || (stream.width() == 640 && stream.height() == 480) || (stream.width() == 480 && stream.height() == 270) || (stream.width() == 424 && stream.height() == 240))
                        {
                            sms->addSubsession(RsServerMediaSubsession::createNew(*env, stream, rsDevice));

                            supported_stream_profiles.push_back(stream);
                            continue;
                        }
                    }
                    else if(stream.fps() == 15 || stream.fps() == 30)
                    {
                        if((stream.width() == 640 && stream.height() == 480) || (stream.width() == 480 && stream.height() == 270) || (stream.width() == 424 && stream.height() == 240))
                        {
                            sms->addSubsession(RsServerMediaSubsession::createNew(*env, stream, rsDevice));
                            supported_stream_profiles.push_back(stream);
                            continue;
                        }
                    }
                    else if(stream.fps() == 60)
                    {
                        if((stream.width() == 480 && stream.height() == 270) || (stream.width() == 424 && stream.height() == 240))
                        {
                            sms->addSubsession(RsServerMediaSubsession::createNew(*env, stream, rsDevice));
                            supported_stream_profiles.push_back(stream);
                            continue;
                        }
                    }
                }
                *env << "Ignoring stream: format: " << stream.format() << " width: " << stream.width() << " height: " << stream.height() << " fps: " << stream.fps() << "\n";
            }

            calculate_extrinsics();

            rtspServer->addServerMediaSession(sms);
            char* url = rtspServer->rtspURL(sms);
            *env << "Play this stream using the URL \"" << url << "\"\n";

            // query camera options
            rtspServer->setSupportedOptions(sensor.getSensorName(), sensor.getSupportedOptions());

            delete[] url;
        }

        env->taskScheduler().doEventLoop(); // does not return
    }

    void calculate_extrinsics()
    {
        for(auto stream_profile_from : supported_stream_profiles)
        {
            for(auto stream_profile_to : supported_stream_profiles)
            {
                int from_sensor_key = RsDevice::getPhysicalSensorUniqueKey(stream_profile_from.stream_type(), stream_profile_from.stream_index());
                int to_sensor_key = RsDevice::getPhysicalSensorUniqueKey(stream_profile_to.stream_type(), stream_profile_to.stream_index());

                rsDevice.get()->minimal_extrinsics_map[std::make_pair(from_sensor_key, to_sensor_key)] = stream_profile_from.get_extrinsics_to(stream_profile_to);
            }
        }
    }

    void sigint_handler(int sig)
    {
        Medium::close(rtspServer);
        env->reclaim();
        env = NULL;
        delete scheduler;
        scheduler = NULL;
        std::cout << "Rs-server downloading\n";
    }

    // Make server a proper singleton
    // Otherwise, initialization of RsDevice is crashing
    // in static build because of order of initialization
    static server& instance()
    {
        static server inst;
        return inst;
    }
};

void sigint_handler(int sig);

int main(int argc, char** argv)
{
    server::instance().main(argc, argv);
    return 0;
}

void sigint_handler(int sig)
{
    server::instance().sigint_handler(sig);
    exit(sig);
}
