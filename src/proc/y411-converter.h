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
        void process_function(byte * const dest[],
            const byte * source,
            int width,
            int height,
            int actual_size,
            int input_size) override;
    };

    void unpack_y411(byte * const dest[], const byte * const s, int w, int h, int actual_size);

#if defined __SSSE3__ && ! defined ANDROID
    void unpack_y411_sse(byte * const dest, const byte * const s, int w, int h, int actual_size);
#endif

    void unpack_y411_native(byte * const dest, const byte * const s, int w, int h, int actual_size);
}
