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
        virtual std::vector<uint8_t> build_command(uint32_t opcode,
            uint32_t param1 = 0,
            uint32_t param2 = 0,
            uint32_t param3 = 0,
            uint32_t param4 = 0,
            uint8_t const * data = nullptr,
            size_t dataLength = 0) const = 0;

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
