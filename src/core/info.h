#pragma once

#include "streaming.h"

#include <vector>

namespace librealsense
{
    class debug_interface : public device_interface
    {
    public:
        virtual std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) = 0;
    };
}
