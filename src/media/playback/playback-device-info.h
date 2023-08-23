// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <src/device-info.h>


namespace librealsense {


class playback_device_info : public device_info
{
    std::string const _filename;

public:
    explicit playback_device_info( std::shared_ptr< context > const & ctx, std::string const & filename )
        : device_info( ctx )
        , _filename( filename )
    {
    }

    std::string const & get_filename() const { return _filename; }

    std::string get_address() const override { return "file://" + _filename; }

    std::shared_ptr< device_interface > create_device() override;

    bool is_same_as( std::shared_ptr< const device_info > const & other ) const override
    {
        if( auto rhs = std::dynamic_pointer_cast< const playback_device_info >( other ) )
            return _filename == rhs->_filename;
        return false;
    }
};


}  // namespace librealsense
