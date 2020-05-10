// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "RsRtpStream.h"

#include <memory>
#include <mutex>
#include <queue>
#include <time.h>

class rs_rtp_callback
{
private:
    std::shared_ptr<rs_rtp_stream> m_rtp_stream;

    int m_stream_uid;

    int arrive_frames_counter;

public:
    rs_rtp_callback(std::shared_ptr<rs_rtp_stream> rtp_stream)
    {
        m_rtp_stream = rtp_stream;
        m_stream_uid = m_rtp_stream.get()->m_rs_stream.uid;
    }

    ~rs_rtp_callback() {};

    void on_frame(unsigned char* buffer, ssize_t size, struct timeval presentationTime)
    {
        m_rtp_stream.get()->insert_frame(new Raw_Frame((char*)buffer, size, presentationTime));
    }

    int arrived_frames()
    {
        return arrive_frames_counter;
    }
};
