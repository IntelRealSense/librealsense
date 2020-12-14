// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef _VIDEO_RTP_SINK_HH
#    include "VideoRTPSink.hh"
#endif

#define RS_PAYLOAD_HEADER_SIZE (4)

class RsVideoRTPSink : public VideoRTPSink {
public:
    static RsVideoRTPSink* createNew(UsageEnvironment& env, Groupsock* RTPgs) { return new RsVideoRTPSink(env, RTPgs); }
    virtual Boolean frameCanAppearAfterPacketStart(unsigned char const* frameStart, unsigned numBytesInFrame) const { return False; };

protected:
    RsVideoRTPSink(UsageEnvironment& env, Groupsock* RTPgs) : VideoRTPSink(env, RTPgs, 96, 90000, "RS"), fFrameNum(0) {
        // Construct our "a=fmtp:" SDP line:
        // ASSERT: sampling != NULL && colorimetry != NULL
        unsigned const fmtpSDPLineMaxSize = 200; // more than enough space
        fFmtpSDPLine = new char[fmtpSDPLineMaxSize];
        sprintf(fFmtpSDPLine, "a=fmtp:%d ", rtpPayloadType());
    };

    virtual ~RsVideoRTPSink() {
        delete[] fFmtpSDPLine;
    };

    char const* auxSDPLine() { return fFmtpSDPLine; };

private: // redefined virtual functions:
    virtual void doSpecialFrameHandling(unsigned fragmentationOffset, unsigned char* frameStart, unsigned numBytesInFrame, struct timeval framePresentationTime, unsigned numRemainingBytes);
    virtual unsigned specialHeaderSize() const;

private:
    char* fFmtpSDPLine;
    uint8_t fFrameNum;
};
