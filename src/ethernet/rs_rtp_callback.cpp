// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "rs_rtp_callback.hh"

#include <iostream>

void rs_rtp_callback::on_frame(unsigned char* buffer, ssize_t size, struct timeval presentationTime)
{
    m_rtp_stream.get()->insert_frame(new Raw_Frame((char*)buffer, size, presentationTime));
}

rs_rtp_callback::~rs_rtp_callback() {}
