// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    class y411_converter : public functional_processing_block
    {
    public:
        y411_converter(rs2_format target_format)
            : functional_processing_block("Y411 Transform", target_format) {};

    protected:
        y411_converter(const char * name, rs2_format target_format)
            : functional_processing_block(
                name, target_format, RS2_STREAM_COLOR, RS2_EXTENSION_VIDEO_FRAME)
        {
            _stream_filter.format = _target_format;
            _stream_filter.stream = _target_stream;
        }

        void process_function(byte * const dest[],
            const byte * source,
            int width,
            int height,
            int actual_size,
            int input_size) override;
    };
    void unpack_y411(byte * const dest[], const byte * s, int w, int h, int actual_size);
    void unpack_y411_sse(byte * const dest, const byte * s, int w, int h, int actual_size);
    void unpack_y411_native(byte * const dest, const byte * s, int w, int h, int actual_size);
}
