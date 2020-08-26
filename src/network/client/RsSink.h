// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "BasicUsageEnvironment.hh"
#include "liveMedia.hh"

#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_internal.hpp>

#include "RsRtpCallback.h"
#include <compression/CompressionFactory.h>

class RsSink : public MediaSink
{
public:
    static RsSink* createNew(UsageEnvironment& t_env,
                             MediaSubsession& t_subsession,
                             rs2_video_stream t_stream, // identifies the kind of data that's being received
                             char const* t_streamId = NULL); // identifies the stream itself (optional)

    void setCallback(rs_rtp_callback* t_callback);

private:
    RsSink(UsageEnvironment& t_env, MediaSubsession& t_subsession, rs2_video_stream t_stream, char const* t_streamId);
    // called only by "createNew()"
    virtual ~RsSink();

    static void afterGettingFrameUid0(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds);
    static void afterGettingFrameUid1(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds);
    static void afterGettingFrameUid2(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds);
    static void afterGettingFrameUid3(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds);
    void afterGettingFrame(unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds);

private:
    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    unsigned char* m_receiveBuffer;
    unsigned char* m_to;
    int m_bufferSize;
    MediaSubsession& m_subsession;
    char* m_streamId;
    FILE* m_fp;

    rs_rtp_callback* m_rtpCallback;
    rs2_video_stream m_stream;
    std::shared_ptr<ICompression> m_iCompress;
    std::vector<FramedSource::afterGettingFunc*> m_afterGettingFunctions;
};
