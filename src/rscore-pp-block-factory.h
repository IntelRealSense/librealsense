// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/pp-block-factory.h"


namespace librealsense {


class rscore_pp_block_factory : public pp_block_factory
{
public:
    std::shared_ptr< processing_block_interface > create_pp_block( std::string const & name,
                                                                   rsutils::json const & settings ) override;
};


}  // namespace librealsense
