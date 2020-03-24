// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsDevice.hh"
#include "RsUsageEnvironment.h"
#include "compression/CompressionFactory.h"
#include "string.h"
#include <BasicUsageEnvironment.hh>
#include <iostream>
#include <math.h>
#include <thread>

RsSensor::RsSensor(UsageEnvironment* t_env, rs2::sensor t_sensor, rs2::device t_device)
    : env(t_env)
    , m_sensor(t_sensor)
    , m_device(t_device)
{
    for(rs2::stream_profile streamProfile : m_sensor.get_stream_profiles())
    {
        if(streamProfile.is<rs2::video_stream_profile>())
        {
            //make a map with all the sensor's stream profiles
            m_streamProfiles.emplace(getStreamProfileKey(streamProfile), streamProfile.as<rs2::video_stream_profile>());
            m_prevSample.emplace(getStreamProfileKey(streamProfile), std::chrono::high_resolution_clock::now());
        }
    }
    m_memPool = new MemoryPool();
}

int RsSensor::open(std::unordered_map<long long int, rs2::frame_queue>& t_streamProfilesQueues)
{
    std::vector<rs2::stream_profile> requestedStreamProfiles;
    for(auto streamProfile : t_streamProfilesQueues)
    {
        //make a vector of all requested stream profiles
        long long int streamProfileKey = streamProfile.first;
        requestedStreamProfiles.push_back(m_streamProfiles.at(streamProfileKey));
        if(CompressionFactory::isCompressionSupported(m_streamProfiles.at(streamProfileKey).format(), m_streamProfiles.at(streamProfileKey).stream_type()))
        {
            rs2::video_stream_profile vsp = m_streamProfiles.at(streamProfileKey);
            std::shared_ptr<ICompression> compressPtr = CompressionFactory::getObject(vsp.width(), vsp.height(), vsp.format(), vsp.stream_type(), RsSensor::getStreamProfileBpp(vsp.format()));
            if(compressPtr != nullptr)
            {
                m_iCompress.insert(std::pair<long long int, std::shared_ptr<ICompression>>(streamProfileKey, compressPtr));
            }
        }
        else
        {
            *env << "unsupported compression format or compression is disabled, continue without compression\n";
        }
    }
    m_sensor.open(requestedStreamProfiles);
    return EXIT_SUCCESS;
}

int RsSensor::close()
{
    m_sensor.close();
    return EXIT_SUCCESS;
}

int RsSensor::stop()
{
    m_sensor.stop();
    return EXIT_SUCCESS;
}

int RsSensor::start(std::unordered_map<long long int, rs2::frame_queue>& t_streamProfilesQueues)
{
    auto callback = [&](const rs2::frame& frame) {
        long long int profileKey = getStreamProfileKey(frame.get_profile());
        //check if profile exists in map:
        if(t_streamProfilesQueues.find(profileKey) != t_streamProfilesQueues.end())
        {
            std::chrono::high_resolution_clock::time_point curSample = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(curSample - m_prevSample[profileKey]);
            if(CompressionFactory::isCompressionSupported(frame.get_profile().format(), frame.get_profile().stream_type()))
            {
                unsigned char* buff = m_memPool->getNextMem();
                int frameSize = m_iCompress.at(profileKey)->compressBuffer((unsigned char*)frame.get_data(), frame.get_data_size(), buff);
                if(frameSize == -1)
                {
                    m_memPool->returnMem(buff);
                    return;
                }
                memcpy((unsigned char*)frame.get_data(), buff, frameSize);
                m_memPool->returnMem(buff);
            }
            //push frame to its queue
            t_streamProfilesQueues[profileKey].enqueue(frame);
            m_prevSample[profileKey] = curSample;
        }
    };
    m_sensor.start(callback);
    return EXIT_SUCCESS;
}

long long int RsSensor::getStreamProfileKey(rs2::stream_profile t_profile)
{
    long long int key;
    key = t_profile.stream_type() * pow(10, 12) + t_profile.format() * pow(10, 10) + t_profile.fps() * pow(10, 8) + t_profile.stream_index();
    if(t_profile.is<rs2::video_stream_profile>())
    {
        rs2::video_stream_profile videoStreamProfile = t_profile.as<rs2::video_stream_profile>();
        key += videoStreamProfile.width() * pow(10, 4) + videoStreamProfile.height();
    }
    return key;
}

std::string RsSensor::getSensorName()
{
    if(m_sensor.supports(RS2_CAMERA_INFO_NAME))
    {
        return std::string(m_sensor.get_info(RS2_CAMERA_INFO_NAME));
    }
    else
    {
        return "Unknown Sensor";
    }
}

int RsSensor::getStreamProfileBpp(rs2_format t_format)
{
    int bpp = 0;
    switch(t_format)
    {
    case RS2_FORMAT_RGB8:
    {
        bpp = 3;
        break;
    }
    case RS2_FORMAT_BGR8:
    {
        bpp = 3;
        break;
    }
    case RS2_FORMAT_RGBA8:
    {
        bpp = 3; //TODO: need to be 4 bpp, change it after add support for 4 bpp formats
        break;
    }
    case RS2_FORMAT_BGRA8:
    {
        bpp = 3; //TODO: need to be 4 bpp, change it after add support for 4 bpp formats
        break;
    }
    case RS2_FORMAT_Z16:
    case RS2_FORMAT_Y16:
    case RS2_FORMAT_Y8:
    case RS2_FORMAT_RAW16:
    case RS2_FORMAT_YUYV:
    case RS2_FORMAT_UYVY:
    {
        bpp = 2;
        break;
    }
    default:
        bpp = 0;
        break;
    }
    return bpp;
}

std::vector<RsOption> RsSensor::getSupportedOptions()
{
    std::vector<RsOption> returnedVector;
    try
    {

        std::vector<rs2_option> options = m_sensor.get_supported_options();
        for(auto opt : options)
        {
            if(!m_sensor.supports(opt))
                continue;

            RsOption option;
            option.m_opt = opt;
            option.m_range = m_sensor.get_option_range(opt);
            returnedVector.push_back(option);
        }
    }
    catch(const std::exception& e)
    {
        *env << e.what() << "\n";
    }
    return returnedVector;
}
