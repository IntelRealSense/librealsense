// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "GroupsockHelper.hh"
#include "Locale.hh"
#include "liveMedia.hh"

#include "RsCommon.h"
#include "RsMediaSession.hh"

#include <ctype.h>
#include <iostream>

// RsMediaSession

RsMediaSession* RsMediaSession::createNew(UsageEnvironment& env, char const* sdpDescription)
{
    RsMediaSession* newSession = new RsMediaSession(env);
    if(newSession != NULL)
    {
        if(!newSession->initializeWithSDP(sdpDescription))
        {
            delete newSession;
            return NULL;
        }
    }
    return newSession;
}

RsMediaSession::RsMediaSession(UsageEnvironment& env)
    : MediaSession(env)
{}

RsMediaSession::~RsMediaSession() {}

MediaSubsession* RsMediaSession::createNewMediaSubsession()
{
    // default implementation:
    return new RsMediaSubsession(*this);
}

// RsMediaSubsessionIterator

RsMediaSubsessionIterator::RsMediaSubsessionIterator(RsMediaSession const& session)
    : fOurSession(session)
{
    reset();
}

RsMediaSubsessionIterator::~RsMediaSubsessionIterator() {}

RsMediaSubsession* RsMediaSubsessionIterator::next()
{
    RsMediaSubsession* result = fNextPtr;

    if(fNextPtr != NULL)
        fNextPtr = (RsMediaSubsession*)(fNextPtr->fNext);

    return result;
}

void RsMediaSubsessionIterator::reset()
{
    fNextPtr = (RsMediaSubsession*)(fOurSession.fSubsessionsHead);
}

// MediaSubsession 

RsMediaSubsession::RsMediaSubsession(RsMediaSession& parent)
    : MediaSubsession(parent)
{}

RsMediaSubsession::~RsMediaSubsession()
{
    if(sink != NULL)
    {
        Medium::close(sink);
    }
}

Boolean RsMediaSubsession::createSourceObjects(int useSpecialRTPoffset)
{
    if(strcmp(fCodecName, RS_PAYLOAD_FORMAT.c_str()) == 0)
    {
        // This subsession uses our custom RTP payload format:
        std::string mimeTypeString = RS_MEDIA_TYPE + "/" + RS_PAYLOAD_FORMAT;
        fReadSource = fRTPSource = SimpleRTPSource::createNew(env(), fRTPSocket, fRTPPayloadFormat, fRTPTimestampFrequency, mimeTypeString.c_str());

#if defined(_WIN32)
        if(rtpSource() != NULL)
        {
            // configure a  time threshold
            // for reordering misordered incoming packets:
            unsigned const thresh = 33000; // 33 ms
            rtpSource()->setPacketReorderingThresholdTime(thresh);

            // Set the RTP source's OS socket buffer size as appropriate
            int socketNum = rtpSource()->RTPgs()->socketNum();
            unsigned curBufferSize = getReceiveBufferSize(env(), socketNum);
            unsigned newBufferSize = 1024 * 1024 * 8; // 8M
            newBufferSize = setReceiveBufferTo(env(), socketNum, newBufferSize);
            env() << "Changed socket receive buffer size for the \"" << mediumName() << "/" << codecName() << "\" subsession from " << curBufferSize << " to " << newBufferSize << " bytes\n";
        }
#endif
        return True;
    }
    else
    {
        // This subsession uses some other RTP payload format - perhaps one that we already implement:
        return MediaSubsession::createSourceObjects(useSpecialRTPoffset);
    }
}
