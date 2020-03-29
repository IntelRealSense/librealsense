// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "synthetic-stream.h"

namespace librealsense
{
    class LRS_EXTENSION_API uvyv_to_yuyv : public functional_processing_block
    {
    public:
        uvyv_to_yuyv();
        

    protected:
        uvyv_to_yuyv(const char* name);

        void process_function(byte* const dest[], const byte* source, int width, int height, int actual_size, int input_size);
    };
}
