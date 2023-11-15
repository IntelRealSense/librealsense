// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include "device-info.h"


namespace librealsense {


class software_device;


class software_device_info : public device_info
{
    std::weak_ptr< software_device > _dev;
    std::string _address;

public:
    explicit software_device_info( std::shared_ptr< context > const & ctx );

    // The usage is dictated by the rs2 APIs: rather than creating the info and then using create_device() to create the
    // device, it's the other way around (see rs2_context_add_software_device).
    //
    void set_device( std::shared_ptr< software_device > const & dev );

    std::string get_address() const override { return _address; }

    bool is_same_as( std::shared_ptr< const device_info > const & other ) const override;

    std::shared_ptr< device_interface > create_device() override;
};


}  // namespace librealsense
