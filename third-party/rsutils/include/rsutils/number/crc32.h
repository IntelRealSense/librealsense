// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <cstdint>
#include <stddef.h>


namespace rsutils {
namespace number {


// Calculates standard 32bit CRC code
uint32_t calc_crc32( const uint8_t * buf, size_t bufsize );


}  // namespace number
}  // namespace rsutils