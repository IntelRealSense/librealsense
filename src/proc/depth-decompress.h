// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "proc/synthetic-stream.h"

namespace librealsense
{
    class LRS_EXTENSION_API depth_decompression_huffman : public functional_processing_block
    {
    public:
        depth_decompression_huffman();

        void process_function(byte* const dest[], const byte* source, int width, int height, int actual_size, int input_size) override;

    protected:
        depth_decompression_huffman(const depth_decompression_huffman&) = delete;
    };

    MAP_EXTENSION(RS2_EXTENSION_DEPTH_HUFFMAN_DECODER, librealsense::depth_decompression_huffman);
}
