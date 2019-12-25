// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <utility>
#include "core/processing.h"
#include "proc/synthetic-stream.h"
#include "source.h"

namespace librealsense
{
    class LRS_EXTENSION_API depth_decompression_huffman : public functional_processing_block
    {
    public:
        depth_decompression_huffman(rs2_format target_format) :
            depth_decompression_huffman("Depth decompression Huffman-code") {};

    protected:
        depth_decompression_huffman(const char* name) :
            functional_processing_block(name, RS2_FORMAT_Z16, RS2_STREAM_DEPTH, RS2_EXTENSION_DEPTH_FRAME) {};
        void process_function(byte* const dest[], const byte* source, int width, int height, int actual_size) override;
    };
}
