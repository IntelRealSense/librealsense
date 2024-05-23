// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <src/serializable-interface.h>


namespace librealsense {


class device_interface;
class sensor_interface;


class dds_serializable : public serializable_interface
{
public:
    std::vector< uint8_t > serialize_json() const override;
    void load_json( const std::string & json_content ) override;

protected:
    virtual device_interface const & get_serializable_device() const = 0;
    virtual std::vector< sensor_interface * > get_serializable_sensors() = 0;
    virtual std::vector< sensor_interface const * > get_serializable_sensors() const = 0;
};


}  // namespace librealsense
