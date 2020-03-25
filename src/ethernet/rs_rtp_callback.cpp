// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "rs_rtp_callback.hh"

#include <iostream>

void rs_rtp_callback::on_frame(unsigned char* buffer, ssize_t size, struct timeval presentationTime)
{
    if (((std::intptr_t)(buffer ) & 0xF) == 0)
                {
                        std::cout<<"on_frame buffer is 16 bit alliened\n";
                }
                else
                {
                    std::cout<<sizeof(RsNetworkHeader)<<"\n";
                    std::cout<<"on_frame buffer is NOT 16 bit alliened\n";
                }
    m_rtp_stream.get()->insert_frame(new Raw_Frame((char*)buffer, size, presentationTime));
}

rs_rtp_callback::~rs_rtp_callback() {}
