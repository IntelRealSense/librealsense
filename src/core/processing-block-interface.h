// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "options.h"
#include "info-interface.h"
#include <src/types.h>

#include <vector>
#include <memory>


namespace librealsense {


class synthetic_source_interface;
class frame_holder;


class processing_block_interface
    : public virtual options_interface
    , public virtual info_interface
{
public:
    virtual ~processing_block_interface() = default;

    virtual void set_processing_callback( frame_processor_callback_ptr callback ) = 0;
    virtual void set_output_callback( frame_callback_ptr callback ) = 0;
    virtual void invoke( frame_holder frame ) = 0;
    virtual synthetic_source_interface & get_source() = 0;
};


using processing_blocks = std::vector< std::shared_ptr< processing_block_interface > >;


}  // namespace librealsense
