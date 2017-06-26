// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "streaming.h"

#include <vector>

namespace rsimpl2
{
    class debug_interface : public virtual device_interface
    {
    public:
        virtual std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) = 0;
    };
}
