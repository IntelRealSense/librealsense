// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#define RS2_NET_MAJOR_VERSION    2
#define RS2_NET_MINOR_VERSION    0
#define RS2_NET_PATCH_VERSION    0

#define COMPRESSION_ENABLED
#define COMPRESSION_ZSTD

#define CHUNK_SIZE (1024*2)
#pragma pack (push, 1)
typedef struct chunk_header{
    uint16_t size;
    uint32_t offset;
    uint32_t index;
    uint8_t  status;
    uint8_t  meta_id;
    uint64_t meta_data;
} chunk_header_t;
#pragma pack(pop)
#define CHUNK_HLEN (sizeof(chunk_header_t))

using StreamIndex     = std::pair<rs2_stream, int>;
using StreamPair      = std::pair<StreamIndex, StreamIndex>;

