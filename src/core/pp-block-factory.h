// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/json-fwd.h>
#include <string>


namespace librealsense {


class processing_block_interface;


// Instantiator of post-processing filters, AKA processing blocks, based on name and settings.
// These are the ones you see in the Viewer, and are also returned by
// sensor_interface::get_recommended_processing_blocks(). Their names are serialized to rosbags when recording and
// reinstantiated on playback.
//
// Do not use directly: the context manages these!
//
class pp_block_factory
{
public:
    virtual ~pp_block_factory() = default;

    // Creates a post-processing block. If the name is unrecognized, a null pointer is returned. Otherwise, if the
    // settings are wrong or some other error condition is encountered, an exception is raised.
    // 
    // The name is case-insensitive.
    //
    virtual std::shared_ptr< processing_block_interface > create_pp_block( std::string const & name,
                                                                           rsutils::json const & settings )
        = 0;
};


}  // namespace librealsense
