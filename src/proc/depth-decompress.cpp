// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <fstream>
#include "../common/decompress-huffman.h"
#include "proc/depth-decompress.h"

namespace librealsense
{
    void depth_decompression_huffman::process_function(byte* const dest[], const byte* source, int width, int height, int actual_size, int input_size)
    {
        if (!unhuffimage4(reinterpret_cast<uint32_t*>(const_cast<byte*>(source)), uint32_t(input_size >> 2), width << 1, height, const_cast<byte*>(*dest)))
        {
            std::stringstream ss;
            ss << "Depth_huffman_decode_error_input_" << input_size << "_output_" << actual_size << ".raw";
            std::string filename = ss.str().c_str();

            LOG_ERROR("Depth decompression failed, dump encoded data for review to" << filename);
            std::ofstream file(filename, std::ios::binary | std::ios::trunc);
            if (!file.good())
                throw std::runtime_error(to_string() << "Invalid binary file specified " << filename << " verify the target path and location permissions");
            file.write((const char*)source, input_size);
        }
    }
}
