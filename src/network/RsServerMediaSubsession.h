// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <liveMedia.hh>

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif

#include <librealsense2/rs.hpp>

#include "RsSensor.h"
#include "RsSource.h"
#include "RsVideoRTPSink.h"

#include <iostream>

class RsServerMediaSubsession : public OnDemandServerMediaSubsession
{
public:
    static RsServerMediaSubsession* createNew(UsageEnvironment& t_env, frames_queue* pfq, rs2::stream_profile stream) { 
        return new RsServerMediaSubsession(t_env, pfq, stream); 
    };

protected:
    RsServerMediaSubsession(UsageEnvironment& t_env, frames_queue* pfq, rs2::stream_profile stream) 
        : OnDemandServerMediaSubsession(t_env, false), m_queue(pfq), m_stream(stream) {};
    virtual ~RsServerMediaSubsession() {};

    // virtual char const* sdpLines() {
    //     // regenerate SDP description to get actual subsession state
    //     if(fSDPLines != NULL) delete [] fSDPLines;
    //     fSDPLines = NULL;
    //     return OnDemandServerMediaSubsession::sdpLines();
    // };

    virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
        static char* privateAuxSDPLine = {0};
        if (privateAuxSDPLine == NULL) privateAuxSDPLine = new char[512]; // more than enough? :)

        const char* auxSDPLine = OnDemandServerMediaSubsession::getAuxSDPLine(rtpSink, inputSource);
        if (auxSDPLine == NULL) auxSDPLine = "";

        sprintf(privateAuxSDPLine, "%sactive=%s;key=%llu\r\n", auxSDPLine, 
            m_queue->is_streaming(m_stream) ? "yes" : "no", slib::profile2key(m_stream)
        );

        // std::stringstream ss;
        // ss << auxSDPLine << "active=";
        // if (m_queue->is_streaming(m_stream)) ss << "yes";
        // else ss << "no";
        // ss << ";key=" << slib::profile2key(m_stream) << "\r\n";

        return privateAuxSDPLine;
    };

    virtual FramedSource* createNewStreamSource(unsigned t_clientSessionId, unsigned& t_estBitrate) {
        t_estBitrate = 40000; // "estBitrate" is the stream's estimated bitrate, in kbps
        std::cout << std::endl << "Creating device source" << std::endl;

#define ENCODER_LZ4

#if   defined(ENCODER_JPEG200)
        RsDeviceSource* rs_source = RsDeviceSource::createNew(envir(), m_queue);
        return JPEG2000EncodeFilter::createNew(envir(), rs_source);
#elif defined(ENCODER_JPEG)
        RsDeviceSource* rs_source = RsDeviceSource::createNew(envir(), m_queue);
        return JPEGEncodeFilter::createNew(envir(), rs_source);
#elif defined(ENCODER_LZ4)
  #if 0
        RsDeviceSource* rs_source = RsDeviceSource::createNew(envir(), m_queue);
        return LZ4EncodeFilter::createNew(envir(), rs_source);
  #else        
        return RsDeviceSource::createNew(envir(), m_queue, m_stream); 
  #endif        
#else
        return RsDeviceSource::createNew(envir(), m_queue);
#endif

    };

    virtual RTPSink* createNewRTPSink(Groupsock* t_rtpGroupsock, unsigned char t_rtpPayloadTypeIfDynamic, FramedSource* t_inputSource) {
        std::string mime(t_inputSource->MIMEtype());
        std::string media = mime.substr(0, mime.find('/'));
        std::string type  = mime.substr(mime.find('/') + 1);
        std::cout << "Source provides " << media << "/" << type << "\n";

        if (type == "JPEG2000") {
            /// JPEG2000 video
            std::cout << "Using JPEG2000VideoRTPSink\n";
            return JPEG2000VideoRTPSink::createNew(envir(), t_rtpGroupsock);
        } else if (type == "JPEG") {
            /// JPEG video
            std::cout << "Using JPEGVideoRTPSink\n";
            return JPEGVideoRTPSink::createNew(envir(), t_rtpGroupsock);
        } else if (type == "RS") {
            /// LZ4 compressed video
            std::cout << "Using RsVideoRTPSink\n";
            return RsVideoRTPSink::createNew(envir(), t_rtpGroupsock);
        } else {
            /// RAW
            std::cout << "Using RawVideoRTPSink\n";
            rs2::video_stream_profile vsp = m_stream.as<rs2::video_stream_profile>();
            return RawVideoRTPSink::createNew(envir(), t_rtpGroupsock, t_rtpPayloadTypeIfDynamic, 
                vsp.width(), vsp.height(), 8, "YCbCr-4:2:2", "BT709-2");
        }
    };

private:
    char* m_pAuxSDPLine;

    frames_queue* m_queue;

    // rs2::sensor        m_sensor;
    rs2::stream_profile m_stream;
};
