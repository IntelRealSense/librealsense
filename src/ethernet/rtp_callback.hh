// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <time.h>

class rtp_callback
{
public:
    void virtual on_frame(unsigned char* buffer, ssize_t size, struct timeval presentationTime) = 0;

};
