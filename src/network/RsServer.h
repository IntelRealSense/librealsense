// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <liveMedia.hh>
#include <GroupsockHelper.hh>
#include <BasicUsageEnvironment.hh>
#include <RTSPCommon.hh>

#include <librealsense2/rs.hpp>

#include <string>

class RsRTSPServer : public RTSPServer {
public:
    static RsRTSPServer* createNew(UsageEnvironment& env, Port port = 554) {
        int socket = setUpOurSocket(env, port);
        if (socket == -1) return NULL;

        return new RsRTSPServer(env, socket, port);
    }

private:
    RsRTSPServer(UsageEnvironment& env, int socket, Port port) : RTSPServer(env, socket, port, NULL, 65) {};
    virtual ~RsRTSPServer() {};

protected:
    char const* allowedCommandNames() {
        return "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER, SET_PARAMETER, LIST, QUERY, GET_OPTION, SET_OPTION";
    };

public:
    class RsRTSPClientConnection: public RTSPServer::RTSPClientConnection {
    protected:
        RsRTSPClientConnection(RsRTSPServer& ourServer, int clientSocket, struct sockaddr_storage const& clientAddr)
                    : RTSPServer::RTSPClientConnection(ourServer, clientSocket, clientAddr), m_parent(ourServer) {};
        virtual ~RsRTSPClientConnection() {};

        friend class RsRTSPServer;
        // catch unsupported commands here
        virtual void handleCmd_bad() {
            // Don't do anything with "fCurrentCSeq", because it might be nonsense
            std::cout << "Bad command" << "\n";
            RTSPServer::RTSPClientConnection::handleCmd_bad();
        };

        virtual void handleHTTPCmd_StreamingGET(char const* urlSuffix, char const* fullRequestStr) {
            std::cout << "GET request for " << urlSuffix << " : " << fullRequestStr << std::endl;
            RTSPServer::RTSPClientConnection::handleHTTPCmd_StreamingGET(urlSuffix, (char const*)fRequestBuffer);
        }

        virtual void handleCmd_notSupported() {
            // Parse the request string into command name and 'CSeq', then handle the command:
            fRequestBuffer[fRequestBytesAlreadySeen] = '\0';
            char cmdName[RTSP_PARAM_STRING_MAX];
            char urlPreSuffix[RTSP_PARAM_STRING_MAX];
            char urlSuffix[RTSP_PARAM_STRING_MAX];
            char cseq[RTSP_PARAM_STRING_MAX];
            char sessionIdStr[RTSP_PARAM_STRING_MAX];
            unsigned contentLength = 0;
            Boolean playAfterSetup = False;
            fLastCRLF[2] = '\0'; // temporarily, for parsing
            Boolean parseSucceeded = parseRTSPRequestString((char*)fRequestBuffer, fLastCRLF + 2 - fRequestBuffer, cmdName, sizeof cmdName, urlPreSuffix, sizeof urlPreSuffix, urlSuffix, sizeof urlSuffix, cseq, sizeof cseq, sessionIdStr, sizeof sessionIdStr, contentLength);
            fLastCRLF[2] = '\r'; // restore its value

            if (strcmp(cmdName, "LIST") == 0) {
                // std::string url;
                // if (urlPreSuffix[0] != '\0') {
                //     url += urlPreSuffix;
                //     url += "/";
                // }
                // url += urlSuffix;
                // ServerMediaSession* session = fOurServer.lookupServerMediaSession(url.c_str());
                // if(session == NULL) {
                //     handleCmd_notFound();
                // } else {
                //     session->incrementReferenceCount();

                    snprintf((char*)fResponseBuffer,
                        sizeof fResponseBuffer,
                        "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
                        "%s"
                //      "Content-Base: %s/\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %lu\r\n\r\n"
                        "%s",
                        fCurrentCSeq,
                        dateHeader(),
                //      fOurRTSPServer.rtspURL(session, fClientInputSocket),
                        m_parent.m_list.length(),
                        m_parent.m_list.c_str());

                //     session->decrementReferenceCount();
                //     if(session->referenceCount() == 0 && session->deleteWhenUnreferenced()) {
                //         fOurServer.removeServerMediaSession(session);
                //     }
                // }
            // } else if (strcmp(cmdName, "QUERY") == 0) {
            //         snprintf((char*)fResponseBuffer,
            //             sizeof fResponseBuffer,
            //             "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
            //             "%s"
            //     //      "Content-Base: %s/\r\n"
            //             "Content-Type: text/plain\r\n"
            //             "Content-Length: %lu\r\n\r\n"
            //             "%s",
            //             fCurrentCSeq,
            //             dateHeader(),
            //     //      fOurRTSPServer.rtspURL(session, fClientInputSocket),
            //             m_parent.m_sdsc.length(),
            //             m_parent.m_sdsc.c_str());

            } else {
                RTSPServer::RTSPClientConnection::handleCmd_notSupported();
            }
        };

        void handleCmd_conflict() {
            setRTSPResponse("409 Conflict");
        };

    private:
        RsRTSPServer& m_parent;
    };

    class RsRTSPClientSession: public RTSPServer::RTSPClientSession {
    protected:
        RsRTSPClientSession(RsRTSPServer& ourServer, u_int32_t sessionId)
                    : RTSPServer::RTSPClientSession(ourServer, sessionId), m_parent(ourServer) {};
        virtual ~RsRTSPClientSession() {};    

        friend class RsRTSPServer;

        // virtual void handleCmd_SETUP(RTSPClientConnection* ourClientConnection, char const* urlPreSuffix, char const* urlSuffix, char const* fullRequestStr) {
        //     std::cout << "SETUP command..." << std::endl;
        //     RTSPServer::RTSPClientSession::handleCmd_SETUP(ourClientConnection, urlPreSuffix, urlSuffix, fullRequestStr);
        //     std::cout << "...for " << urlSuffix << std::endl;

        //     for (int trackNum = 0; trackNum < fNumStreamStates; ++trackNum) {
        //         auto subsession = fStreamStates[trackNum].subsession;
        //         if (subsession != NULL && strcmp(urlSuffix, subsession->trackId()) == 0) {
        //             // got the subsession
        //         }
        //     }

        // };

        virtual void handleCmd_PLAY(RTSPClientConnection* ourClientConnection, ServerMediaSubsession* subsession, char const* fullRequestStr) {
            std::cout << "PLAY command" << std::endl;
            RTSPServer::RTSPClientSession::handleCmd_PLAY(ourClientConnection, subsession, fullRequestStr);
        };

        virtual void handleCmd_TEARDOWN(RTSPClientConnection* ourClientConnection, ServerMediaSubsession* subsession) {
            std::cout << "TEARDOWN command" << std::endl;
            RTSPServer::RTSPClientSession::handleCmd_TEARDOWN(ourClientConnection, subsession);
        };

    private:
        RsRTSPServer& m_parent;
    };
     
protected: // redefined virtual functions
    friend class RsRTSPClientConnection;
    friend class RsRTSPClientSession;

    virtual ClientConnection* createNewClientConnection(int clientSocket, struct sockaddr_storage const& clientAddr) {
        return new RsRTSPClientConnection(*this, clientSocket, clientAddr);
    }

    virtual ClientSession* createNewClientSession(u_int32_t sessionId) {
        return new RsRTSPClientSession(*this, sessionId);
    };


private:
    std::string m_list; // cameras served by server
};

class server
{
public:
    server(rs2::device dev, std::string addr, int port);
    ~server();

    void start();
    void stop();

private:
    UsageEnvironment* env;
    TaskScheduler* scheduler;

    RsRTSPServer* srv;

    // std::vector<rs2::video_stream_profile> supported_stream_profiles; // streams for extrinsics map creation

    unsigned int port;
    std::string m_serial;
    std::string m_name;

    std::thread m_httpd;
    void doHTTP();

    std::string m_sensors_desc; // sensors description
};
