// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <api.h>
#include <librealsense2-net/rs_net.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <functional>

#include <RsStreamLib.h>
#include <RsRTSPClient.h>
#include <RsSink.h>

using namespace std::placeholders;

void RSRTSPClient::shutdownStream() {
    // First, check whether any subsessions have still to be closed:
    if (m_session != NULL) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*m_session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL) {
            if (subsession->sink != NULL) {
                Medium::close(subsession->sink);
                subsession->sink = NULL;
                if (subsession->rtcpInstance() != NULL) {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
                }
                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive) {
            // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
            // Don't bother handling the response to the "TEARDOWN".
            sendTeardownCommand(*m_session, NULL);
        }

        m_session->close(m_session);
        m_session = NULL;
    }

    LOG_INFO("Closing the stream.");
    Medium::close(this);
}

// static callback
void RSRTSPClient::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    return ((RSRTSPClient*)rtspClient)->continueAfterDESCRIBE(resultCode, resultString);
}

// member
void RSRTSPClient::continueAfterDESCRIBE(int resultCode, char* resultString) {
    if (resultCode != 0) {
        LOG_ERROR("Failed to get a SDP description: " << resultString);
        delete[] resultString;
        throw std::runtime_error("Failed to get a SDP description");
    } else {
        m_sdp = std::string(resultString);
        delete[] resultString; // because we don't need it anymore
    }

    prepareSession();
    playSession();
}

void RSRTSPClient::prepareSession() {
    UsageEnvironment& env = envir(); // alias

    // Create a media session object from this SDP description:
    LOG_DEBUG("SDP description:\n" << m_sdp);
    m_session = RsMediaSession::createNew(env, m_sdp.c_str());

    if (m_session == NULL) {
        LOG_ERROR("Failed to create a MediaSession object from the SDP description: " << env.getResultMsg());
        throw std::runtime_error("Malformed server response");
    } else if (!m_session->hasSubsessions()) {
        LOG_ERROR("This session has no media subsessions (i.e., no \"m=\" lines)");
        throw std::runtime_error("No profiles found");
    } else {
        LOG_INFO("Session created " << m_session->name() << "/" << m_session->sessionDescription() << "/" << m_session->controlPath());
        return;
    }

    // An unrecoverable error occurred with this stream.
    shutdownStream();
}

void RSRTSPClient::playSession() {
    // setup streams first
    if (m_streams_it == NULL) m_streams_it = new std::map<uint64_t, rs_net_stream*>::iterator(m_streams.begin());

    while (*m_streams_it != m_streams.end()) {
        rs2::stream_profile profile = (*m_streams_it)->second->get_profile();
        uint64_t profile_key = slib::profile2key(profile);

        // check for fake formats for color streams
        if (profile.stream_type() == RS2_STREAM_COLOR) {
            profile_key = slib::profile2key(profile, RS2_FORMAT_YUYV);
        }

        bool profile_found = false;
        MediaSubsessionIterator it(*m_session);

        LOG_INFO("Looking  for " << slib::profile2key(profile) << "\t" << slib::print_profile(profile) << " => " << profile_key);
        while (m_subsession = it.next()) {
            uint64_t subsession_key = std::stoull(m_subsession->attrVal_str("key"));
            rs2_video_stream vs = slib::key2stream(subsession_key);
            LOG_INFO("Checking for " << subsession_key << "\t" << slib::print_stream(&vs));
            if (profile_key == subsession_key) {
                LOG_INFO("Profile match for " << m_subsession->controlPath());
                profile_found = true;

                int useSpecialRTPoffset = -1; // for supported codecs
                if (strcmp(m_subsession->codecName(), "RS") == 0) {
                    useSpecialRTPoffset = 0;
                }

                if (!m_subsession->initiate(useSpecialRTPoffset)) {
                    LOG_ERROR("Failed to initiate the \"" << m_subsession->controlPath() << "\" subsession: " << envir().getResultMsg());
                } else {
                    std::stringstream ss;
                    ss << "Initiated the '" << std::setw(10) << m_subsession->controlPath() << "' " 
                                                << m_subsession->mediumName()   << "/" 
                                                << m_subsession->protocolName() << "/" 
                                                << m_subsession->videoWidth() << "x" << m_subsession->videoHeight() << "x" << m_subsession->videoFPS() << " subsession (";
                    if (m_subsession->rtcpIsMuxed()) {
                        ss << "client port " << m_subsession->clientPortNum();
                    } else {
                        ss << "client ports " << m_subsession->clientPortNum() << "-" << m_subsession->clientPortNum() + 1;
                    }
                    ss << ") [" << m_subsession->readSource()->name() << " : " << m_subsession->readSource()->MIMEtype() << "]";
                    LOG_INFO(ss.str());

                    // Continue setting up this subsession, by sending a RTSP "SETUP" command:
                    sendSetupCommand(*m_subsession, RSRTSPClient::continueAfterSETUP);
                    return;
                }
            }
        }
        if (!profile_found) {
            LOG_ERROR("Cannot match a profile");
            throw std::runtime_error("Cannot match a profile");
        }
    }

    delete m_streams_it;
    m_streams_it = NULL;

    sendPlayCommand(*m_session, continueAfterPLAY);
}

// static callback
void RSRTSPClient::continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    return ((RSRTSPClient*)rtspClient)->continueAfterSETUP(resultCode, resultString);
}

// member
void RSRTSPClient::continueAfterSETUP(int resultCode, char* resultString)
{
    UsageEnvironment& env = envir(); // alias
    
    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)
    m_subsession->sink = RSSink::createNew(env, *m_subsession, url(), (*m_streams_it)->second->get_queue(), m_subsession->videoWidth() * m_subsession->videoHeight() * m_subsession->videoFPS());

    // do not wait for the out of order packets
    // m_scs.subsession->rtpSource()->setPacketReorderingThresholdTime(0); 

    if (m_subsession->sink == NULL) {
        LOG_ERROR("Failed to create a data sink for the '" << m_subsession->controlPath() << "' subsession: " << env.getResultMsg());
    } else {
        LOG_DEBUG("Created a data sink for the \"" << m_subsession->controlPath() << "\" subsession");
        m_subsession->miscPtr = this; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        m_subsession->sink->startPlaying(*(m_subsession->readSource()), subsessionAfterPlaying, m_subsession);
        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (m_subsession->rtcpInstance() != NULL) {
            m_subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, m_subsession);
        }
    }

    // setup the remainded streams
    (*m_streams_it)++;
    playSession();
}

void RSRTSPClient::playStreams(std::map<uint64_t, rs_net_stream*> streams) {
    m_streams = streams;
    sendDescribeCommand(RSRTSPClient::continueAfterDESCRIBE);
}

// static callback
void RSRTSPClient::continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
    return ((RSRTSPClient*)rtspClient)->continueAfterPLAY(resultCode, resultString);
}

// member
void RSRTSPClient::continueAfterPLAY(int resultCode, char* resultString) {
    UsageEnvironment& env = envir(); // alias

    if (resultCode != 0) {
        LOG_ERROR("Failed to start playing session: " << resultString);

        // An unrecoverable error occurred with this stream.
        shutdownStream();
    } else {
        LOG_INFO("Started playing session.");
    }

    delete[] resultString;
}

void RSRTSPClient::subsessionAfterPlaying(void* clientData)
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RSRTSPClient* rtspClient = (RSRTSPClient*)(subsession->miscPtr);

    LOG_INFO("Closing " << subsession->controlPath() << "session");

    // Begin by closing this subsession's stream:
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while((subsession = iter.next()) != NULL)
    {
        if(subsession->sink != NULL)
            return; // this subsession is still active
    }

    // All subsessions' streams have now been closed, so shutdown the client:
    rtspClient->shutdownStream();
}

void RSRTSPClient::subsessionByeHandler(void* clientData, char const* reason)
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RSRTSPClient* rtspClient = (RSRTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    std::stringstream ss;
    ss << "Received RTCP \"BYE\"";
    if(reason != NULL)
    {
        ss << " (reason:\"" << reason << "\")";
        delete[](char*) reason;
    }
    ss << " on \"" << subsession->controlPath() << "\" subsession";
    LOG_INFO(ss.str());

    // Now act as if the subsession had closed:
    rtspClient->subsessionAfterPlaying(subsession);
}
