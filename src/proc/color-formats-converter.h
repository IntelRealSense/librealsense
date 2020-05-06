// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    class LRS_EXTENSION_API color_converter : public functional_processing_block
    {
    protected:
        color_converter(const char* name, rs2_format target_format, rs2_stream target_stream = RS2_STREAM_COLOR) :
            functional_processing_block(name, target_format, target_stream, RS2_EXTENSION_VIDEO_FRAME) {};
    };

    class LRS_EXTENSION_API yuy2_converter : public color_converter
    {
    public:
        yuy2_converter(rs2_format target_format) :
            yuy2_converter("YUY Converter", target_format) {};

    protected:
        yuy2_converter(const char* name, rs2_format target_format) :
            color_converter(name, target_format) {};
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };

    class LRS_EXTENSION_API uyvy_converter : public color_converter
    {
    public:
        uyvy_converter(rs2_format target_format, rs2_stream target_stream = RS2_STREAM_COLOR) :
            uyvy_converter("UYVY Converter", target_format, target_stream) {};

    protected:
        uyvy_converter(const char* name, rs2_format target_format, rs2_stream target_stream) :
            color_converter(name, target_format, target_stream) {};
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };

    class LRS_EXTENSION_API mjpeg_converter : public color_converter
    {
    public:
        mjpeg_converter(rs2_format target_format) :
            mjpeg_converter("MJPEG Converter", target_format) {};

    protected:
        mjpeg_converter(const char* name, rs2_format target_format) :
            color_converter(name, target_format) {};
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };

    class LRS_EXTENSION_API bgr_to_rgb : public color_converter
    {
    public:
        bgr_to_rgb() :
            bgr_to_rgb("BGR to RGB Converter") {};

    protected:
        bgr_to_rgb(const char* name) :
            color_converter(name, RS2_FORMAT_RGB8, RS2_STREAM_INFRARED) {};
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };
}
