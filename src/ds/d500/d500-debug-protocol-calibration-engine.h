// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-private.h"
#include <src/hw-monitor.h>
#include <src/calibration-engine-interface.h>

namespace librealsense
{

class debug_interface;
class d500_debug_protocol_calibration_engine : public calibration_engine_interface
{
public:
    d500_debug_protocol_calibration_engine(debug_interface* dev) : _dev(dev){}
    virtual std::vector<uint8_t> get(uint32_t opcode, 
        uint32_t param1 = 0, 
        uint32_t param2 = 0, 
        uint32_t param3 = 0, 
        uint32_t param4 = 0) const override;
    virtual std::vector<uint8_t> set(uint32_t opcode,
        uint32_t param1 = 0, 
        uint32_t param2 = 0, 
        uint32_t param3 = 0, 
        uint32_t param4 = 0,
        uint8_t const* data = nullptr, 
        size_t dataLength = 0) override;

private:
    debug_interface* _dev;
};

} // namespace librealsense
