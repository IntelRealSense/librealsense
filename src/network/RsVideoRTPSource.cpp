// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsVideoRTPSource.h"

#include <iostream>

RsVideoRTPSource* RsVideoRTPSource::createNew(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat, unsigned rtpTimestampFrequency, char const* sampling)
{
    return new RsVideoRTPSource(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency, sampling);
}

RsVideoRTPSource ::RsVideoRTPSource(UsageEnvironment& env, Groupsock* RTPgs, unsigned char rtpPayloadFormat, unsigned rtpTimestampFrequency, char const* sampling)
    : MultiFramedRTPSource(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency)
{
    fSampling = strDup(sampling);
}

RsVideoRTPSource::~RsVideoRTPSource()
{
    delete[] fSampling;
}

#define RS_PAYLOAD_HEADER_SIZE 4

Boolean RsVideoRTPSource ::processSpecialHeader(BufferedPacket* packet, unsigned& resultSpecialHeaderSize)
{
    unsigned char* headerStart = packet->data();
    unsigned packetSize = packet->dataSize();

    // There should be enough space for a payload header:
    if(packetSize < RS_PAYLOAD_HEADER_SIZE)
        return False;

    u_int32_t fragmentOffset = (headerStart[3] << 16) | (headerStart[2] << 8) | (headerStart[1]);
    fCurrentPacketBeginsFrame = fragmentOffset == 0;
    fCurrentPacketCompletesFrame = packet->rtpMarkerBit();

    resultSpecialHeaderSize = RS_PAYLOAD_HEADER_SIZE;
    return True;
}

char const* RsVideoRTPSource::MIMEtype() const
{
    return "video/RS";
}
