// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "RsDevice.hh"
#include "ServerMediaSession.hh"

class RsServerMediaSession : public ServerMediaSession
{
public:
    static RsServerMediaSession* createNew(UsageEnvironment& t_env, RsSensor& t_sensor, char const* t_streamName = NULL, char const* t_info = NULL, char const* t_description = NULL, Boolean t_isSSM = False, char const* t_miscSDPLines = NULL);
    RsSensor& getRsSensor();
    void openRsCamera(std::unordered_map<long long int, rs2::frame_queue>& t_streamProfiles);
    void closeRsCamera();

protected:
    RsServerMediaSession(UsageEnvironment& t_env, RsSensor& t_sensor, char const* t_streamName, char const* t_info, char const* t_description, Boolean t_isSSM, char const* t_miscSDPLines);
    virtual ~RsServerMediaSession();

private:
    RsSensor m_rsSensor;
    bool m_isActive;
};
