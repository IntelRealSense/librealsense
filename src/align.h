// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/processing.h"
#include "source.h"

namespace librealsense
{
    class processing_block : public processing_block_interface
    {
    public:
        processing_block(std::size_t output_size, std::shared_ptr<uvc::time_service> ts)
            : _output_size(output_size), _source(ts) {}


        void set_processing_callback(frame_processor_callback callback) override;
        void set_output_callback(frame_callback_ptr callback) override;
        void invoke(frame_holder frame) override;

       

    private:
        callback_source _source;
        frame_callback_ptr _on_result;
        std::size_t _output_size;
    };
}
