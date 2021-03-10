// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsServerMediaSession.h"

// ServerMediaSession

RsServerMediaSession* RsServerMediaSession ::createNew(UsageEnvironment& t_env, RsSensor& t_sensor, char const* t_streamName, char const* t_info, char const* t_description, Boolean t_isSSM, char const* t_miscSDPLines)
{
    return new RsServerMediaSession(t_env, t_sensor, t_streamName, t_info, t_description, t_isSSM, t_miscSDPLines);
}

RsServerMediaSession::RsServerMediaSession(UsageEnvironment& t_env, RsSensor& t_sensor, char const* t_streamName, char const* t_info, char const* t_description, Boolean t_isSSM, char const* t_miscSDPLines)
    : ServerMediaSession(t_env, t_streamName, t_info, t_description, t_isSSM, t_miscSDPLines)
    , m_rsSensor(t_sensor)
    , m_isActive(false)
{}

RsServerMediaSession::~RsServerMediaSession() {}

void RsServerMediaSession::openRsCamera(std::unordered_map<long long int, rs2::frame_queue>& t_streamProfiles)
{
    if(m_isActive)
    {
        envir() << "sensor is already open, closing sensor and than open again...\n";
        closeRsCamera();
    }
    m_rsSensor.open(t_streamProfiles);
    m_rsSensor.start(t_streamProfiles);
    m_isActive = true;
}

void RsServerMediaSession::closeRsCamera()
{
    if(m_isActive)
    {
        m_rsSensor.getRsSensor().stop();
        m_rsSensor.getRsSensor().close();
        m_isActive = false;
    }
}

RsSensor& RsServerMediaSession::getRsSensor()
{
    return m_rsSensor;
}
