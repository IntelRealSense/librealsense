// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <cstdint>


#pragma pack( push, 1 )

// The header structure for eth config
struct eth_config_header
{
    uint16_t version;
    uint16_t size;  // without header
    uint32_t crc;   // without header
};

#pragma pack( pop )
