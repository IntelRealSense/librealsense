// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <src/ds/d500/d500-debug-protocol-calibration-engine.h>
#include "d500-device.h"



namespace librealsense
{

using namespace ds;

std::vector<uint8_t> d500_debug_protocol_calibration_engine::get(uint32_t opcode, uint32_t param1, uint32_t param2, uint32_t param3,
    uint32_t param4) const
{
    // prepare command
    auto cmd = _dev->build_command(opcode, param1, param2, param3, param4);

    // send command to device and get response (calibration config entry + header)
    auto res = _dev->send_receive_raw_data(cmd);

    // slicing 4 first bytes - opcode
    res.erase(res.begin(), res.begin() + 4);

    return res;
}

std::vector<uint8_t>  d500_debug_protocol_calibration_engine::set(uint32_t opcode, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4,
    uint8_t const* data, size_t dataLength)
{
    // prepare command
    auto cmd = _dev->build_command(opcode, param1, param2, param3, param4, data, dataLength);

    // send command to device and get response (calibration config entry + header)
    return _dev->send_receive_raw_data(cmd);
}

}// namespace librealsense