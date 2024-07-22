// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>


namespace librealsense
{

class calibration_engine_interface
{
public:
    virtual std::vector<uint8_t> get(uint32_t opcode, uint32_t param1, uint32_t param2, uint32_t param3, 
        uint32_t param4) const = 0;
    virtual std::vector<uint8_t>  set(uint32_t opcode, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4,
        uint8_t const* data, size_t dataLength) = 0;
};

} // namespace librealsense
