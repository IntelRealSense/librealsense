// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsSink.h"
#include <RsCommon.h>
#include <RsNetDevLog.h>

#define WRITE_FRAMES_TO_FILE 0

RsSink* RsSink::createNew(UsageEnvironment& t_env, MediaSubsession& t_subsession, rs2_video_stream t_stream, char const* t_streamId)
{
    return new RsSink(t_env, t_subsession, t_stream, t_streamId);
}

RsSink::RsSink(UsageEnvironment& t_env, MediaSubsession& t_subsession, rs2_video_stream t_stream, char const* t_streamId)
    : MediaSink(t_env),
    m_subsession(t_subsession), 
    m_receiveBuffer(nullptr),
    m_to(nullptr),
    m_stream(t_stream)
{
    m_streamId = strDup(t_streamId);
    m_bufferSize = t_stream.width * t_stream.height * t_stream.bpp + sizeof(RsFrameHeader);

    m_afterGettingFunctions.push_back(afterGettingFrameUid0);
    m_afterGettingFunctions.push_back(afterGettingFrameUid1);
    m_afterGettingFunctions.push_back(afterGettingFrameUid2);
    m_afterGettingFunctions.push_back(afterGettingFrameUid3);

    if(CompressionFactory::isCompressionSupported(m_stream.fmt, m_stream.type))
    {
        m_iCompress = CompressionFactory::getObject(m_stream.width, m_stream.height, m_stream.fmt, m_stream.type, m_stream.bpp);
    }
    else
    {
        INF << "compression is disabled or configured unsupported format to zip, run without compression";
    }
}

RsSink::~RsSink()
{
    if(m_receiveBuffer != nullptr)
    {
        delete [] m_receiveBuffer;
    }
    delete[] m_streamId;
}

void RsSink::afterGettingFrameUid0(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds)
{

    RsSink* sink = (RsSink*)t_clientData;
    sink->afterGettingFrame(t_frameSize, t_numTruncatedBytes, t_presentationTime, t_durationInMicroseconds);
}

void RsSink::afterGettingFrameUid1(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds)
{

    RsSink* sink = (RsSink*)t_clientData;
    sink->afterGettingFrame(t_frameSize, t_numTruncatedBytes, t_presentationTime, t_durationInMicroseconds);
}

void RsSink::afterGettingFrameUid2(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds)
{

    RsSink* sink = (RsSink*)t_clientData;
    sink->afterGettingFrame(t_frameSize, t_numTruncatedBytes, t_presentationTime, t_durationInMicroseconds);
}

void RsSink::afterGettingFrameUid3(void* t_clientData, unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned t_durationInMicroseconds)
{

    RsSink* sink = (RsSink*)t_clientData;
    sink->afterGettingFrame(t_frameSize, t_numTruncatedBytes, t_presentationTime, t_durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0

void RsSink::afterGettingFrame(unsigned t_frameSize, unsigned t_numTruncatedBytes, struct timeval t_presentationTime, unsigned /*t_durationInMicroseconds*/)
{
    RsNetworkHeader* header = (RsNetworkHeader*)m_receiveBuffer;
    if(header->data.frameSize == t_frameSize - sizeof(RsNetworkHeader))
    {
        if(this->m_rtpCallback != NULL)
        {
            if(CompressionFactory::isCompressionSupported(m_stream.fmt, m_stream.type) && m_iCompress != nullptr)
            {
                m_to = new unsigned char[MAX_MESSAGE_SIZE];
                if(m_to == nullptr)
                {
                    return;
                }
                int decompressedSize = m_iCompress->decompressBuffer(m_receiveBuffer + sizeof(RsFrameHeader), header->data.frameSize - sizeof(RsMetadataHeader), m_to + sizeof(RsFrameHeader));
                if(decompressedSize != -1)
                {
                    // copy metadata
                    memcpy(m_to + sizeof(RsNetworkHeader), m_receiveBuffer + sizeof(RsNetworkHeader), sizeof(RsMetadataHeader));
                    this->m_rtpCallback->on_frame((u_int8_t*)m_to + sizeof(RsNetworkHeader), decompressedSize + sizeof(RsMetadataHeader), t_presentationTime);
                }
                delete [] m_receiveBuffer;
            }
            else
            {
                this->m_rtpCallback->on_frame(m_receiveBuffer + sizeof(RsNetworkHeader), header->data.frameSize, t_presentationTime);
            }
        }
        else
        {
            // TODO: error, no call back
            delete [] m_receiveBuffer;
            envir() << "Frame call back is NULL\n";
        }
    }
    else
    {
        envir() << m_streamId << ":corrupted frame!!!: data size is " << header->data.frameSize << " frame size is " << t_frameSize << "\n";
        delete [] m_receiveBuffer;
    }
    m_receiveBuffer = nullptr;

    // Then continue, to request the next frame of data
    continuePlaying();
}

Boolean RsSink::continuePlaying()
{
    if (fSource == NULL)
    {
        return False; // sanity check (should not happen)
    }

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    m_receiveBuffer = new unsigned char[MAX_MESSAGE_SIZE];
    if(m_receiveBuffer == nullptr)
    {
        return false;
    }

    if(m_stream.uid >= 0 && m_stream.uid < m_afterGettingFunctions.size())
    {
        fSource->getNextFrame(m_receiveBuffer, m_bufferSize, m_afterGettingFunctions.at(m_stream.uid), this, onSourceClosure, this);
    }
    else
    {
        return false;
    }

    return True;
}

void RsSink::setCallback(rs_rtp_callback* t_callback)
{
    this->m_rtpCallback = t_callback;
}
