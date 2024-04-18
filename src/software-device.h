// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.
#pragma once

#include "device.h"
#include "core/extension.h"
#include <librealsense2/hpp/rs_types.hpp>


namespace librealsense  {


class software_sensor;


class software_device : public device
{
public:
    software_device( std::shared_ptr< const device_info > const & );
    virtual ~software_device();

    software_sensor& add_software_sensor(const std::string& name);
    software_sensor& get_software_sensor( size_t index);

    void set_matcher_type(rs2_matchers matcher);

    std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

    std::vector< tagged_profile > get_profiles_tags() const override { return {}; };
    
    void register_extrinsic(const stream_interface& stream);

    using destruction_callback_ptr = std::shared_ptr< rs2_software_device_destruction_callback >;
    void register_destruction_callback( destruction_callback_ptr );

protected:
    std::vector<std::shared_ptr<software_sensor>> _software_sensors;
    destruction_callback_ptr _user_destruction_callback;
    rs2_matchers _matcher = RS2_MATCHER_DEFAULT;
};

MAP_EXTENSION(RS2_EXTENSION_SOFTWARE_DEVICE, software_device);


}  // namespace librealsense
