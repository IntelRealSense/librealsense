// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>

namespace rsutils
{
    namespace number
    {
        uint8_t reverse_bits(uint8_t b);
        void reverse_byte_array_bits(uint8_t* byte_array, int size);
    }
}
