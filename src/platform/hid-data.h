// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <cstdint>


namespace librealsense {


#pragma pack( push, 1 )
struct hid_data
{
    int32_t x;
    //uint16_t reserved1[2];
    int32_t y;
    //uint16_t reserved2[2];
    int32_t z;
    //uint16_t reserved3[2];
};


struct hid_mipi_data
{
    uint8_t typeID;
    uint8_t skip1;
    uint64_t hwTs;
    int16_t x;
    int16_t y;
    int16_t z;
    uint64_t hwTs2;
    uint64_t skip2;
};
#pragma pack( pop )


}  // namespace librealsense
