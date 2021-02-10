// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <option.h>
#include <software-device.h>
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <list>
#include <sstream>

#include "RsNetStream.h"
#include "RsMediaSession.h"

class RSRTSPClient : public RTSPClient {
public:
    static RSRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL) {
        return new RSRTSPClient(env, rtspURL);
    }

protected:
    RSRTSPClient(UsageEnvironment& env, char const* rtspURL)
        : RTSPClient(env, rtspURL, 0, "", 0, -1), m_streams_it(NULL) {}

    virtual ~RSRTSPClient() {};

public:
    void playStreams(std::map<uint64_t, rs_net_stream*> streams);

    void shutdownStream();

    void prepareSession();
    void playSession();

    static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString); // callback
    void continueAfterDESCRIBE(int resultCode, char* resultString); // member

    static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString); // callback
    void continueAfterSETUP(int resultCode, char* resultString); // member

    static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString); // callback
    void continueAfterPLAY(int resultCode, char* resultString); // member

    static void subsessionAfterPlaying(void* clientData);
    static void subsessionByeHandler(void* clientData, char const* reason);
    static void streamTimerHandler(void* clientData);

private:
    RsMediaSession*  m_session;
    MediaSubsession* m_subsession;

    std::string m_sdp;

    std::map<uint64_t, rs_net_stream*> m_streams;
    std::map<uint64_t, rs_net_stream*>::iterator* m_streams_it;
};
