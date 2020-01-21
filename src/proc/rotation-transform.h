// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    // Processes rotated frames.
    class rotation_transform : public functional_processing_block
    {
    public:
        rotation_transform(rs2_format target_format, rs2_stream target_stream, rs2_extension extension_type);
        rotation_transform(const char* name, rs2_format target_format, rs2_stream target_stream, rs2_extension extension_type);

    protected:
        void init_profiles_info(const rs2::frame* f) override;
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };

    class confidence_rotation_transform : public rotation_transform
    {
    public:
        confidence_rotation_transform();

    protected:
        confidence_rotation_transform(const char* name);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };
}
