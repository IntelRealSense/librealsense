// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"
#include "option.h"
#include "image.h"

namespace librealsense
{
    class y12i_to_y16y16 : public interleaved_functional_processing_block
    {
    public:
        y12i_to_y16y16(int left_idx = 1, int right_idx = 2);

    protected:
        y12i_to_y16y16(const char* name, int left_idx, int right_idx);
        void process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size) override;
    };
}
