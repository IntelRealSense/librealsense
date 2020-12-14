// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef _MULTI_FRAMED_RTP_SOURCE_HH
#include "MultiFramedRTPSource.hh"
#endif

class RsVideoRTPSource: public MultiFramedRTPSource {
public:
    static RsVideoRTPSource* createNew(UsageEnvironment& env, Groupsock* RTPgs,
    unsigned char rtpPayloadFormat,
    unsigned rtpTimestampFrequency,
    char const* sampling);

protected:
    virtual ~RsVideoRTPSource();

protected:
    RsVideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs,
        unsigned char rtpPayloadFormat,
        unsigned rtpTimestampFrequency,
        char const* sampling);
    // called only by createNew()

private:
    // redefined virtual functions:
    virtual Boolean processSpecialHeader(BufferedPacket* packet, unsigned& resultSpecialHeaderSize);
    virtual char const* MIMEtype() const;

private:
    char* fSampling;
};
