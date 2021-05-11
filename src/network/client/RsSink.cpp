// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <api.h>
#include <librealsense2-net/rs_net.h>

#include "RsNetCommon.h"
#include "RsSink.h"

// Implementation of "RSSink":
#define RS_SINK_RECEIVE_BUFFER_SIZE (CHUNK_SIZE + CHUNK_HLEN + 20)

RSSink* RSSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId, std::shared_ptr<SafeQueue> q, uint32_t threshold)
{
    return new RSSink(env, subsession, streamId, q, threshold);
}

RSSink::RSSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId, std::shared_ptr<SafeQueue> q, uint32_t threshold)
    : MediaSink(env), fSubsession(subsession), m_threshold(threshold), m_frames(q)
{
    fStreamId = strDup(streamId);
    std::shared_ptr<uint8_t> chunk(new u_int8_t[RS_SINK_RECEIVE_BUFFER_SIZE]);
    m_chunk = chunk;
}

RSSink::~RSSink()
{
    delete[] fStreamId;
}

void RSSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds)
{
    RSSink* sink = (RSSink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void RSSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
    m_frames->push(m_chunk);
    std::shared_ptr<uint8_t> chunk(new u_int8_t[RS_SINK_RECEIVE_BUFFER_SIZE]);
    m_chunk = chunk;
 
    // Then continue, to request the next frame of data:
    continuePlaying();
}

void RSSink::getNextFrame(RSSink* sink) {
    sink->continuePlaying();
}

Boolean RSSink::continuePlaying() {
    if(fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(m_chunk.get(), RS_SINK_RECEIVE_BUFFER_SIZE, afterGettingFrame, this, onSourceClosure, this);
    return True;
}
