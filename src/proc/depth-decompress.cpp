// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "../common/decompress-huffman.h"
#include "proc/depth-decompress.h"

namespace librealsense
{
    void depth_decompression_huffman::process_function(byte* const dest[], const byte* source, int width, int height, int actual_size)
    {
        //unpack_mjpeg(dest, source, width, height, actual_size);
        //unhuffimage4(uint32_t * compressedImage, uint32_t lengthOfCompressedImageInU32s, uint32_t widthInBytes, uint32_t height, unsigned char* image)

        if (!unhuffimage4(reinterpret_cast<uint32_t*>(const_cast<byte*>(source)), uint32_t(actual_size >> 2), width << 1, height, const_cast<byte*>(*dest)))
        {
            LOG_ERROR("Depth decompression failed");
        }
            //reinterpret_cast<uint32_t*>(const_cast<void*>(f.pixels)),
            //static_cast<int>(f.frame_size >> 2),
            /*mode.profile.width << 1,
            mode.profile.height,
            dest.data()[0]))*/
    }
}
