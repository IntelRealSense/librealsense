// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "RsVideoRTPSink.h"

void RsVideoRTPSink::doSpecialFrameHandling(unsigned fragmentationOffset, unsigned char* frameStart, 
                                             unsigned numBytesInFrame, struct timeval framePresentationTime, unsigned numRemainingBytes)
{
    // std::cout << "doSpecialFrameHandling processing packet for frame " << (void*)frameStart << " with size of " << numBytesInFrame << " offset " << fragmentationOffset << " and " << numRemainingBytes << " bytes reminding" << std::endl;

    if (fragmentationOffset == 0) {
        fFrameNum++;
    }

    // our special header specifies the frame and offset
    unsigned char header[RS_PAYLOAD_HEADER_SIZE] = {0};
    header[0] = fFrameNum;
    header[1] = fragmentationOffset & 0xff;
    header[2] = (fragmentationOffset >> 8 ) & 0xff;
    header[3] = (fragmentationOffset >> 16) & 0xff;
    
    setSpecialHeaderBytes((uint8_t*)&header, sizeof(header));

    if(numRemainingBytes == 0)
    {
        // This packet contains the last (or only) fragment of the frame.
        // Set the RTP 'M' ('marker') bit:
        setMarkerBit();
    }

    // Also set the RTP timestamp:
    setTimestamp(framePresentationTime);
}

unsigned RsVideoRTPSink::specialHeaderSize() const
{
    return RS_PAYLOAD_HEADER_SIZE;
}
