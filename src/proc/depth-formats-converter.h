// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"
#include "option.h"
#include "image.h"

namespace librealsense
{
    class inzi_converter : public interleaved_functional_processing_block
    {
    public:
        inzi_converter(rs2_format target_ir_format) :
            inzi_converter("INZI to depth and IR Transform", target_ir_format) {};

    protected:
        inzi_converter(const char* name, rs2_format target_ir_format);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };

    class invi_converter : public functional_processing_block
    {
    public:
        invi_converter(rs2_format target_format) :
            invi_converter("INVI to IR Transform", target_format) {};

    protected:
        invi_converter(const char* name, rs2_format target_format) :
            functional_processing_block(name, target_format, RS2_STREAM_INFRARED, RS2_EXTENSION_VIDEO_FRAME) {};
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };

    class w10_converter : public functional_processing_block
    {
    public:
        w10_converter(const rs2_format& target_format) :
            w10_converter("W10 Transform", target_format) {};

    protected:
        w10_converter(const char* name, const rs2_format& target_format);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };
}
