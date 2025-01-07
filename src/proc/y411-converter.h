// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    class LRS_EXTENSION_API y411_converter : public functional_processing_block
    {
    public:
        y411_converter(rs2_format target_format)
            : functional_processing_block("Y411 Transform", target_format) {};

    protected:
        void process_function( uint8_t * const dest[],
            const uint8_t * source,
            int width,
            int height,
            int actual_size,
            int input_size) override;
    };

    void unpack_y411( uint8_t * const dest[], const uint8_t * const s, int w, int h, int actual_size);

#if defined __SSSE3__ && ! defined ANDROID
    void unpack_y411_sse( uint8_t * const dest, const uint8_t * const s, int w, int h, int actual_size);
#endif

    void unpack_y411_native( uint8_t * const dest, const uint8_t * const s, int w, int h, int actual_size);
}
