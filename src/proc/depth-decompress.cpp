// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <fstream>
#include "../common/decompress-huffman.h"
#include "proc/depth-decompress.h"
#include "environment.h"

namespace librealsense
{
    depth_decompression_huffman::depth_decompression_huffman():
        functional_processing_block("Depth Huffman Decoder", RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME)
    {
        get_option(RS2_OPTION_STREAM_FILTER).set(RS2_STREAM_DEPTH);
        get_option(RS2_OPTION_STREAM_FORMAT_FILTER).set(RS2_FORMAT_Z16H);
    }

    void depth_decompression_huffman::process_function(byte* const dest[], const byte* source, int width, int height, int actual_size, int input_size)
    {
        if (!unhuffimage4(reinterpret_cast<uint32_t*>(const_cast<byte*>(source)), uint32_t(input_size >> 2), width << 1, height, const_cast<byte*>(*dest)))
        {
            LOG_INFO("Depth decompression failed, ts: " << static_cast<uint64_t>(environment::get_instance().get_time_service()->get_time())
                        << " , compressed size: " << input_size);
        }
    }
}
