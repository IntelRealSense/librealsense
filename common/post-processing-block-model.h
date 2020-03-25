// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "processing-block-model.h"
#include "post-processing-filter.h"


namespace rs2
{
    /*
        A less generic processing block that's used specifically for object detection
        so that the filters inside are all of type object_detection_filter.
    */
    class post_processing_block_model : public processing_block_model
    {
    public:
        post_processing_block_model(
            subdevice_model * owner,
            std::shared_ptr< post_processing_filter > block,
            std::function<rs2::frame( rs2::frame )> invoker,
            std::string& error_message,
            bool enabled = true
        )
            : processing_block_model( owner, block->get_name(), block, invoker, error_message, enabled )
        {
        }

    protected:
        void processing_block_enable_disable( bool actual ) override
        {
            dynamic_cast<post_processing_filter *>(_block.get())->on_processing_block_enable( actual );
        }
    };
}


