// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#ifndef _STREAM_CLIENT_STATE_H
#define _STREAM_CLIENT_STATE_H

#include "RsMediaSession.hh"

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState
{
public:
    StreamClientState()
        : m_session(NULL)
    {}
    virtual ~StreamClientState()
    {
        Medium::close(m_session);
    }

public:
    RsMediaSession* m_session;
};

#endif // _STREAM_CLIENT_STATE_H
