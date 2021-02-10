// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <option.h>
#include <software-device.h>
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <list>
#include <sstream>

#include "RsSafeQueue.h"

class RSSink : public MediaSink {
public:
    static RSSink* createNew(UsageEnvironment& env,
                                MediaSubsession& subsession, // identifies the kind of data that's being received
                                char const* streamId,        // identifies the stream itself
                                SafeQueue* q,
                                uint32_t threshold);         // number of stored bytes for the drop

private:
    RSSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamIdm, SafeQueue* q, uint32_t threshold); // called only by "createNew()"
    virtual ~RSSink();

    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds); // callback
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds); // member

private:
    static void getNextFrame(RSSink* sink);

    // redefined virtual functions:
    virtual Boolean continuePlaying();

private:
    uint32_t m_threshold;

    uint8_t* fReceiveBuffer;
    
    SafeQueue* m_frames;
    
    MediaSubsession& fSubsession;
    char* fStreamId;
};
