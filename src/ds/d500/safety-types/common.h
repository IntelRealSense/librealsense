// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>

namespace librealsense
{

#pragma pack(push, 1)

    struct table_header
    {
        uint16_t version;       // major.minor. Big-endian
        uint16_t table_type;    // type
        uint32_t table_size;    // full size including: header footer
        uint32_t calib_version; // major.minor.index
        uint32_t crc32;         // crc of all the data in table excluding this header/CRC
    };

#pragma pack(pop)

}
