// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "MediaSession.hh"

class RsMediaSubsession; // forward

class RsMediaSession : public MediaSession
{
public:
    static RsMediaSession* createNew(UsageEnvironment& env, char const* sdpDescription);

protected:
    RsMediaSession(UsageEnvironment& env);
    // called only by createNew();
    virtual ~RsMediaSession();

    virtual MediaSubsession* createNewMediaSubsession();

    friend class RsMediaSubsessionIterator;
};

class RsMediaSubsessionIterator
{
public:
    RsMediaSubsessionIterator(RsMediaSession const& session);
    virtual ~RsMediaSubsessionIterator();

    RsMediaSubsession* next(); // NULL if none
    void reset();

private:
    RsMediaSession const& fOurSession;
    RsMediaSubsession* fNextPtr;
};

class RsMediaSubsession : public MediaSubsession
{
protected:
    friend class RsMediaSession;
    friend class RsMediaSubsessionIterator;
    RsMediaSubsession(RsMediaSession& parent);
    virtual ~RsMediaSubsession();
    virtual Boolean createSourceObjects(int useSpecialRTPoffset);
    // create "fRTPSource" and "fReadSource" member objects, after we've been initialized via SDP
};
