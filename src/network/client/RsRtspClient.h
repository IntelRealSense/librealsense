// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"

#include "RsMediaSession.h"
#include "RsRtsp.h"
#include <RsCommon.h>

#include <librealsense2/hpp/rs_internal.hpp>

#include <condition_variable>
#include <map>
#include <vector>

//TODO: check if this timeout is reasonable for all commands
#define RTSP_CLIENT_COMMANDS_TIMEOUT_SEC 10

#define SDP_EXTRINSICS_ARGS 13

enum RsRtspReturnCode
{
    OK,
    ERROR_GENERAL,
    ERROR_NETWROK,
    ERROR_TIME_OUT,
    ERROR_WRONG_FLOW
};

struct RsRtspReturnValue
{
    RsRtspReturnCode exit_code;
    std::string msg;
};

class StreamClientState
{
public:
    StreamClientState() : m_session(NULL) {}
    virtual ~StreamClientState()
    {
        Medium::close(m_session);
    }

public:
    RsMediaSession* m_session;
};

class RsRTSPClient : public RTSPClient, RsRtsp
{
public:
    static RsRtsp* createNew(char const* t_rtspURL, char const* t_applicationName, portNumBits t_tunnelOverHTTPPortNum, int idx);
    void describe();
    void setup(rs2_video_stream t_stream);
    void initFunc();

    static long long int getStreamProfileUniqueKey(rs2_video_stream t_profile);
    static int getPhysicalSensorUniqueKey(rs2_stream stream_type, int sensors_index);
    void setDeviceData(DeviceData t_data);

    // IcamOERtsp functions
    virtual std::vector<rs2_video_stream> getStreams();
    virtual int addStream(rs2_video_stream t_stream, rs_rtp_callback* t_frameCallBack);
    virtual int start();
    virtual int stop();
    virtual int close();
    virtual int getOption(const std::string& t_sensorName, rs2_option t_option, float& t_value);
    virtual int setOption(const std::string& t_sensorName, rs2_option t_option, float t_value);
    void setGetParamResponse(float t_res);
    virtual DeviceData getDeviceData()
    {
        return m_deviceData;
    }
    virtual std::vector<IpDeviceControlData> getControls();

    static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterTEARDOWN(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterPAUSE(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterOPTIONS(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterSETCOMMAND(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void continueAfterGETCOMMAND(RTSPClient* rtspClient, int resultCode, char* resultString);
    static void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
    static void subsessionByeHandler(void* clientData, char const* reason);
    char& getEventLoopWatchVariable()
    {
        return m_eventLoopWatchVariable;
    };
    std::mutex& getTaskSchedulerMutex()
    {
        return m_taskSchedulerMutex;
    };

    unsigned sendSetParameterCommand(responseHandler* responseHandler, char const* parameterName, char const* parameterValue, Authenticator* authenticator = NULL);
    unsigned sendGetParameterCommand(responseHandler* responseHandler, char const* parameterName, Authenticator* authenticator = NULL);
    Boolean setRequestFields(RequestRecord* request, char*& cmdURL, Boolean& cmdURLWasAllocated, char const*& protocolStr, char*& extraHeaders, Boolean& extraHeadersWereAllocated);

private:
    RsRTSPClient(TaskScheduler* t_scheduler, UsageEnvironment* t_env, char const* t_rtspURL, int t_verbosityLevel, char const* t_applicationName, portNumBits t_tunnelOverHTTPPortNum, int idx);

    // called only by createNew();
    virtual ~RsRTSPClient();

    StreamClientState m_scs;
    bool isActiveSession = false; //this flag should affect the get/set param commands to run in context of specific session, currently value is always false
    std::vector<rs2_video_stream> m_supportedProfiles;
    std::map<long long int, RsMediaSubsession*> m_subsessionMap;
    RsRtspReturnValue m_lastReturnValue;
    static int m_streamCounter;
    // TODO: should we have seperate mutex for each command?
    std::condition_variable m_cv;
    std::mutex m_commandMtx;
    bool m_commandDone = false;
    DeviceData m_deviceData;
    float m_getParamRes;
    TaskScheduler* m_scheduler;
    UsageEnvironment* m_env;
    char m_eventLoopWatchVariable = 0;
    std::mutex m_taskSchedulerMutex;

    int m_idx;
};
