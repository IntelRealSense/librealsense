// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "processing-block-interface.h"
#include "extension.h"

#include <functional>
#include <vector>


namespace librealsense {


class recommended_proccesing_blocks_interface
{
public:
    virtual ~recommended_proccesing_blocks_interface() = default;

    virtual processing_blocks get_recommended_processing_blocks() const = 0;
};

MAP_EXTENSION( RS2_EXTENSION_RECOMMENDED_FILTERS, librealsense::recommended_proccesing_blocks_interface );


class recommended_proccesing_blocks_snapshot
    : public recommended_proccesing_blocks_interface
    , public extension_snapshot
{
public:
    recommended_proccesing_blocks_snapshot( const processing_blocks & blocks )
        : _blocks( blocks )
    {
    }

    virtual processing_blocks get_recommended_processing_blocks() const override { return _blocks; }

    void update( std::shared_ptr< extension_snapshot > ext ) override {}

    processing_blocks _blocks;
};


}  // namespace librealsense
