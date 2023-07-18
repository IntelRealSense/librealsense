// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/core/streaming.h>
#include <src/proc/processing-blocks-factory.h>


namespace librealsense {


// librealsense initializes each device type with a device specific information - what are the device raw formats and
// to what profiles we can convert them, what are the default profiles, intrinsics information etc...
// This data is stored in the specific device and sensor classes constructor or initialization functions.
// Currently we did not want to send it as a realdds supported topic, so this class stores all the data needed for DDS
// devices in one place.
class dds_rs_internal_data
{
public:
    static std::vector< tagged_profile > get_profiles_tags( const std::string & product_id,
                                                            const std::string & product_line );
    static std::vector< processing_block_factory > get_profile_converters( const std::string & product_id,
                                                                           const std::string & product_line );
};


}  // namespace librealsense
