// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RTSPServer.hh"
#include "RTSPCommon.hh"
#include "RTSPRegisterSender.hh"
#include "Base64.hh"
#include <GroupsockHelper.hh>
#include "RsRTSPServer.hh"
#include "RsServerMediaSession.h"
#include "RsServerMediaSubsession.h"
#include "librealsense2/hpp/rs_options.hpp"
#include <ipDeviceCommon/RsCommon.h>
#include "RsUsageEnvironment.h"

// RTSPServer implementation

RsRTSPServer* RsRTSPServer::createNew(UsageEnvironment& t_env, std::shared_ptr<RsDevice> t_device, Port t_ourPort, UserAuthenticationDatabase* t_authDatabase, unsigned t_reclamationSeconds)
{
    int ourSocket = setUpOurSocket(t_env, t_ourPort);
    if(ourSocket == -1)
        return NULL;

    return new RsRTSPServer(t_env, t_device, ourSocket, t_ourPort, t_authDatabase, t_reclamationSeconds);
}

RsRTSPServer::RsRTSPServer(UsageEnvironment& t_env, std::shared_ptr<RsDevice> t_device, int t_ourSocket, Port t_ourPort, UserAuthenticationDatabase* t_authDatabase, unsigned t_reclamationSeconds)
    : RTSPServer(t_env, t_ourSocket, t_ourPort, t_authDatabase, t_reclamationSeconds)
    , m_device(t_device)
{}

RsRTSPServer::~RsRTSPServer() {}

std::string getOptionString(rs2_option t_opt, float t_min, float t_max, float t_def, float t_step)
{
    std::ostringstream oss;
    oss << (int)t_opt << "{" << t_min << "," << t_max << "," << t_def << "," << t_step << "}"
        << ";";
    return oss.str();
}

char const* RsRTSPServer::allowedCommandNames()
{
    m_supportedOptionsStr.clear();
    for(const auto& optionsPair : m_supportedOptions)
    {
        m_supportedOptionsStr.append(optionsPair.first);
        m_supportedOptionsStr.append("[");
        for(auto option : optionsPair.second)
        {
            m_supportedOptionsStr.append(getOptionString(option.m_opt, option.m_range.min, option.m_range.max, option.m_range.def, option.m_range.step));
        }
        m_supportedOptionsStr.append("]");
    }
    return m_supportedOptionsStr.c_str();
}

void RsRTSPServer::setSupportedOptions(std::string t_key, std::vector<RsOption> t_supportedOptions)
{
    m_supportedOptions.insert(std::pair<std::string, std::vector<RsOption>>(t_key, t_supportedOptions));
}

// RTSPServer::RTSPClientConnection implementation

RsRTSPServer::RsRTSPClientConnection ::RsRTSPClientConnection(RsRTSPServer& t_ourServer, int t_clientSocket, struct sockaddr_in t_clientAddr)
    : RTSPClientConnection(t_ourServer, t_clientSocket, t_clientAddr)
    , m_fOurRsRTSPServer(t_ourServer)
{}

RsRTSPServer::RsRTSPClientConnection::~RsRTSPClientConnection() {}

void RsRTSPServer::RsRTSPClientConnection::handleCmd_GET_PARAMETER(char const* t_fullRequestStr)
{
    std::ostringstream oss;
    std::vector<RsSensor> sensors;
    std::string str(t_fullRequestStr);
    std::string ContentLength("Content-Length:");
    std::string afterSplit;

    afterSplit = str.substr(str.find(ContentLength) + ContentLength.size());
    char* contLength = strtok((char*)afterSplit.c_str(), "\r\n: ");
    char* sensorName = strtok(NULL, "_\r\n:");
    char* option = strtok(NULL, "\r\n:");
    sensors = m_fOurRsRTSPServer.m_device.get()->getSensors();
    for(auto sensor : sensors)
    {
        if(sensor.getSensorName().compare(sensorName) == 0)
        {
            try
            {
                float value = sensor.getRsSensor().get_option((rs2_option)stoi(std::string(option)));
                char* paramString = new char[strlen(sensorName) + 1 + strlen(option) + strlen(std::to_string(value).c_str()) + 10];
                sprintf(paramString, "%s_%s: %s\r\n", sensorName, option, std::to_string(value).c_str());
                envir() << "GET_PARAMETER: sensor '" << sensorName << "', option '" << option << "', value " << value << "\n";
                setRTSPResponse("200 OK", paramString);
                return;
            }
            catch(const std::exception& e)
            {
                std::string error("500 " + std::string(e.what()));
                setRTSPResponse(error.c_str());
                return;
            }
        }
    }
    setRTSPResponse("500 Invalid Option");
}

void RsRTSPServer::RsRTSPClientConnection::handleCmd_SET_PARAMETER(char const* t_fullRequestStr)
{
    std::ostringstream oss;
    std::vector<RsSensor> sensors;
    std::string str(t_fullRequestStr);
    std::string ContentLength("Content-Length:");
    std::string afterSplit; //, opt, val;

    afterSplit = str.substr(str.find(ContentLength) + ContentLength.size());
    char* contLength = strtok((char*)afterSplit.c_str(), "\r\n: ");
    char* sensorName = strtok(NULL, "_\r\n:");
    char* option = strtok(NULL, "\r\n:");
    char* value = strtok(NULL, "\r\n:");

    envir() << "SET_PARAMETER: sensor '" << sensorName << "', option '" << option << "', value " << value << "\n";
    sensors = m_fOurRsRTSPServer.m_device.get()->getSensors();
    for(auto sensor : sensors)
    {
        if(sensor.getSensorName().compare(sensorName) == 0)
        {
            try
            {
                sensor.getRsSensor().set_option((rs2_option)stoi(std::string(option)), stof(std::string(value)));
                setRTSPResponse("200 OK");
                return;
            }
            catch(const std::exception& e)
            {
                std::string error("500 " + std::string(e.what()));
                setRTSPResponse(error.c_str());
                return;
            }
        }
    }
    setRTSPResponse("500 Invalid Option");
}

// RsRTSPServer::RsRTSPClientSession implementation

RsRTSPServer::RsRTSPClientSession ::RsRTSPClientSession(RTSPServer& t_ourServer, u_int32_t t_sessionId)
    : RTSPClientSession(t_ourServer, t_sessionId)
{}

RsRTSPServer::RsRTSPClientSession::~RsRTSPClientSession() {}

void RsRTSPServer::RsRTSPClientSession::handleCmd_TEARDOWN(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession)
{
    envir() << "TEARDOWN \n";
    try
    {
        closeRsCamera();
    }
    catch(const std::exception& e)
    {
        std::string error("500 " + std::string(e.what()));
        setRTSPResponse(t_ourClientConnection, error.c_str());
        return;
    }

    RTSPServer::RTSPClientSession::handleCmd_TEARDOWN(t_ourClientConnection, t_subsession);
}

void RsRTSPServer::RsRTSPClientSession::handleCmd_PLAY(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession, char const* t_fullRequestStr)
{
    envir() << "PLAY \n";
    try
    {
        openRsCamera();
    }
    catch(const std::exception& e)
    {
        std::string error("500 " + std::string(e.what()));
        setRTSPResponse(t_ourClientConnection, error.c_str());
        return;
    }

    RTSPServer::RTSPClientSession::handleCmd_PLAY(t_ourClientConnection, t_subsession, t_fullRequestStr);
}

void RsRTSPServer::RsRTSPClientSession::handleCmd_PAUSE(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession)
{
    envir() << "PAUSE \n";
    RTSPServer::RTSPClientSession::handleCmd_PAUSE(t_ourClientConnection, t_subsession);
    try
    {
        closeRsCamera();
    }
    catch(const std::exception& e)
    {
        std::string error("500 " + std::string(e.what()));
        setRTSPResponse(t_ourClientConnection, error.c_str());
        return;
    }
}
void RsRTSPServer::RsRTSPClientSession::handleCmd_GET_PARAMETER(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession, char const* t_fullRequestStr)
{
    std::ostringstream oss;
    std::vector<RsSensor> sensors;
    std::string str(t_fullRequestStr);
    std::string ContentLength("Content-Length:");
    std::string afterSplit; //, opt, val;

    envir() << "GET_PARAMETER\n";
    afterSplit = str.substr(str.find(ContentLength) + ContentLength.size());
    char* contLength = strtok((char*)afterSplit.c_str(), "\r\n: ");
    char* sensorName = strtok(NULL, "_\r\n:");
    char* option = strtok(NULL, "\r\n:");
    try
    {
        float value = static_cast<RsServerMediaSession*>(fOurServerMediaSession)->getRsSensor().getRsSensor().get_option((rs2_option)stoi(std::string(option)));
        char* paramString = new char[strlen(sensorName) + 1 + strlen(option) + strlen(std::to_string(value).c_str()) + 10];
        sprintf(paramString, "%s_%s: %s\r\n", sensorName, option, std::to_string(value).c_str());
        setRTSPResponse(t_ourClientConnection, "200 OK", paramString);
    }
    catch(const std::exception& e)
    {
        std::string error("500 " + std::string(e.what()));
        setRTSPResponse(t_ourClientConnection, error.c_str());
        return;
    }
}

void RsRTSPServer::RsRTSPClientSession::handleCmd_SET_PARAMETER(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession, char const* t_fullRequestStr)
{
    std::ostringstream oss;
    std::vector<RsSensor> sensors;
    std::string str(t_fullRequestStr);
    std::string ContentLength("Content-Length:");
    std::string afterSplit; //, opt, val;

    envir() << "SET_PARAMETER \n";
    afterSplit = str.substr(str.find(ContentLength) + ContentLength.size());
    char* contLength = strtok((char*)afterSplit.c_str(), "\r\n: ");
    char* sensorName = strtok(NULL, "_\r\n:");
    char* option = strtok(NULL, "\r\n:");
    char* value = strtok(NULL, "\r\n:");

    try
    {
        static_cast<RsServerMediaSession*>(fOurServerMediaSession)->getRsSensor().getRsSensor().set_option((rs2_option)stoi(std::string(option)), stof(std::string(value)));
        setRTSPResponse(t_ourClientConnection, "200 OK");
    }
    catch(const std::exception& e)
    {
        std::string error("500 " + std::string(e.what()));
        setRTSPResponse(t_ourClientConnection, error.c_str());
        return;
    }
}
void RsRTSPServer::RsRTSPClientSession::handleCmd_SETUP(RTSPServer::RTSPClientConnection* t_ourClientConnection, char const* t_urlPreSuffix, char const* t_urlSuffix, char const* t_fullRequestStr)
{
    RTSPServer::RTSPClientSession::handleCmd_SETUP(t_ourClientConnection, t_urlPreSuffix, t_urlSuffix, t_fullRequestStr);
    ServerMediaSubsession* subsession;
    if(t_urlSuffix[0] != '\0' && strcmp(fOurServerMediaSession->streamName(), t_urlPreSuffix) == 0)
    {
        // Non-aggregated operation.
        // Look up the media subsession whose track id is "urlSuffix":
        ServerMediaSubsessionIterator iter(*fOurServerMediaSession);
        while((subsession = iter.next()) != NULL)
        {
            if(strcmp(subsession->trackId(), t_urlSuffix) == 0)
            {
                long long int profileKey = static_cast<RsServerMediaSession*>(fOurServerMediaSession)->getRsSensor().getStreamProfileKey(((RsServerMediaSubsession*)(subsession))->getStreamProfile());
                m_streamProfiles[profileKey] = ((RsServerMediaSubsession*)(subsession))->getFrameQueue();
                break; // success
            }
        }
    }
}

void RsRTSPServer::RsRTSPClientSession::openRsCamera()
{
    static_cast<RsServerMediaSession*>(fOurServerMediaSession)->openRsCamera(m_streamProfiles);
}

void RsRTSPServer::RsRTSPClientSession::closeRsCamera()
{
    ((RsServerMediaSession*)fOurServerMediaSession)->closeRsCamera();
    for(int i = 0; i < fNumStreamStates; ++i)
    {
        if(fStreamStates[i].subsession != NULL)
        {
            long long int profile_key = static_cast<RsServerMediaSession*>(fOurServerMediaSession)->getRsSensor().getStreamProfileKey(((RsServerMediaSubsession*)(fStreamStates[i].subsession))->getStreamProfile());
            emptyStreamProfileQueue(profile_key);
        }
    }
}

void RsRTSPServer::RsRTSPClientSession::emptyStreamProfileQueue(long long int profile_key)
{
    rs2::frame f;
    if(m_streamProfiles.find(profile_key) != m_streamProfiles.end())
    {
        while(m_streamProfiles[profile_key].poll_for_frame(&f))
            ;
    }
}

GenericMediaServer::ClientConnection* RsRTSPServer::createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
{
    return new RsRTSPClientConnection(*this, clientSocket, clientAddr);
}

GenericMediaServer::ClientSession* RsRTSPServer::createNewClientSession(u_int32_t t_sessionId)
{
    return new RsRTSPClientSession(*this, t_sessionId);
}
