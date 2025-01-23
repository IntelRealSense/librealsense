// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"
#include "option.h"
#include "image.h"

namespace librealsense
{
    class y16i_10msb_to_y16y16 : public interleaved_functional_processing_block
    {
    public:
        y16i_10msb_to_y16y16(int left_idx = 1, int right_idx = 2);

    protected:
        y16i_10msb_to_y16y16(const char* name, int left_idx, int right_idx);
        void process_function( uint8_t * const dest[], const uint8_t * source, int width, int height, int actual_size, int input_size) override;
    };
}

