// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    class y16_10msb_to_y16 : public functional_processing_block
    {
    public:
        y16_10msb_to_y16();

    protected:
        void process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int actual_size, int input_size ) override;
    };
}

