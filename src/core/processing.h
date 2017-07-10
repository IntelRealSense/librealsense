// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "streaming.h"

namespace librealsense
{
    class processing_block_interface
    {
    public:
        virtual void set_processing_callback(frame_processor_callback callback) = 0;
        virtual void set_output_callback(frame_callback_ptr callback) = 0;
        virtual void invoke(frame_holder frame) = 0;

        virtual ~processing_block_interface() = default;
    };
}
