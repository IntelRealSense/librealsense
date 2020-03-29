// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "uvyv_to_yuyv.h"

#include "stream.h"

namespace librealsense
{
    uvyv_to_yuyv::uvyv_to_yuyv() : 
        uvyv_to_yuyv("uvyv_to_yuyv convertor"){}

    uvyv_to_yuyv::uvyv_to_yuyv(const char* name) :
        functional_processing_block(name, RS2_FORMAT_YUYV) {}


    void uvyv_to_yuyv::process_function(byte* const dest[], const byte* source, 
        int width, int height, int actual_size, int input_size)
    {
        
    }
} // namespace librealsense