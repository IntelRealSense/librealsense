// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "streaming.h"
#include "extension.h"
#include <vector>

namespace librealsense
{
    class debug_interface : public recordable<debug_interface>
    {
    public:
        virtual std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) = 0;
        virtual std::vector<uint8_t> build_raw_data(const uint32_t opcode,
            const uint32_t param1 = 0,
            const uint32_t param2 = 0,
            const uint32_t param3 = 0,
            const uint32_t param4 = 0,
            const std::vector<uint8_t>& data = std::vector<uint8_t>()) = 0;

    };

    MAP_EXTENSION(RS2_EXTENSION_DEBUG, librealsense::debug_interface);

    class debug_snapshot : public extension_snapshot, public debug_interface
    {
    public:
        debug_snapshot(const std::vector<uint8_t>& input) : m_data(input)
        {
        }

    private:
        std::vector<uint8_t> m_data;
    };


}
