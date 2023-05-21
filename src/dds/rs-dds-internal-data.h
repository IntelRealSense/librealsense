// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/core/streaming.h>
#include <src/proc/processing-blocks-factory.h>


namespace librealsense {

class dds_rs_internal_data
{
public:
    static std::vector< tagged_profile > get_profiles_tags( std::string product_id, std::string product_line );
    static std::vector< processing_block_factory > get_profile_converters( std::string product_id,
                                                                           std::string product_line );
};


}  // namespace librealsense
