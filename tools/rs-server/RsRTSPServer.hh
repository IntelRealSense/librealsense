// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "RsDevice.hh"
#include <librealsense2/rs.hpp>

#include <map>

class RsRTSPServer : public RTSPServer
{
public:
    static RsRTSPServer* createNew(UsageEnvironment& t_env, std::shared_ptr<RsDevice> t_device, Port t_ourPort = 554, UserAuthenticationDatabase* t_authDatabase = NULL, unsigned t_reclamationSeconds = 0);
    void setSupportedOptions(std::string t_key, std::vector<RsOption> t_supportedOptions);

protected:
    RsRTSPServer(UsageEnvironment& t_env, std::shared_ptr<RsDevice> t_device, int t_ourSocket, Port t_ourPort, UserAuthenticationDatabase* t_authDatabase, unsigned t_reclamationSeconds);
    virtual ~RsRTSPServer();
    char const* allowedCommandNames();

    std::map<std::string, std::vector<RsOption>> m_supportedOptions;
    std::string m_supportedOptionsStr;
    std::shared_ptr<RsDevice> m_device;

public:
    class RsRTSPClientSession; // forward
    class RsRTSPClientConnection : public RTSPClientConnection
    {

    protected:
        RsRTSPClientConnection(RsRTSPServer& t_ourServer, int t_clientSocket, struct sockaddr_in t_clientAddr);
        virtual ~RsRTSPClientConnection();
        virtual void handleCmd_GET_PARAMETER(char const* fullRequestStr);
        virtual void handleCmd_SET_PARAMETER(char const* fullRequestStr);

        RsRTSPServer& m_fOurRsRTSPServer;

        friend class RsRTSPServer;
        friend class RsRTSPClientSession;
    };
    // The state of an individual client session (using one or more sequential TCP connections) handled by a RTSP server:
    class RsRTSPClientSession : public RTSPClientSession
    {
    protected:
        RsRTSPClientSession(RTSPServer& t_ourServer, u_int32_t t_sessionId);
        virtual ~RsRTSPClientSession();

        friend class RsRTSPServer;
        friend class RsRTSPClientConnection;

        virtual void handleCmd_TEARDOWN(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession);
        virtual void handleCmd_PLAY(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession, char const* t_fullRequestStr);
        virtual void handleCmd_PAUSE(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession);
        virtual void handleCmd_GET_PARAMETER(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession, char const* t_fullRequestStr);
        virtual void handleCmd_SET_PARAMETER(RTSPClientConnection* t_ourClientConnection, ServerMediaSubsession* t_subsession, char const* t_fullRequestStr);

        virtual void handleCmd_SETUP(RTSPServer::RTSPClientConnection* t_ourClientConnection, char const* t_urlPreSuffix, char const* t_urlSuffix, char const* t_fullRequestStr);

        void openRsCamera();
        void closeRsCamera();
        void emptyStreamProfileQueue(long long int t_profile_key);

    private:
        std::unordered_map<long long int, rs2::frame_queue> m_streamProfiles;
    };

protected:
    virtual ClientConnection* createNewClientConnection(int t_clientSocket, struct sockaddr_in t_clientAddr);

protected:
    virtual ClientSession* createNewClientSession(u_int32_t t_sessionId);

private:
    int openRsCamera(RsSensor t_sensor, std::unordered_map<long long int, rs2::frame_queue>& t_streamProfiles);

private:
    friend class RsRTSPClientConnection;
    friend class RsRTSPClientSession;
};