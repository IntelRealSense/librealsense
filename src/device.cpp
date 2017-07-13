// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"

using namespace librealsense;
#include <functional>


int device::add_sensor(std::shared_ptr<sensor_interface> sensor_base)
{
    _sensors.push_back(sensor_base);
    return (int)_sensors.size() - 1;
}

uvc_sensor& device::get_uvc_sensor(int sub)
{
    return dynamic_cast<uvc_sensor&>(*_sensors[sub]);
}

size_t device::get_sensors_count() const
{
    return static_cast<unsigned int>(_sensors.size());
}

sensor_interface& device::get_sensor(size_t subdevice)
{
    try
    {
        return *(_sensors.at(subdevice));
    }
    catch (std::out_of_range)
    {
        throw invalid_value_exception("invalid subdevice value");
    }
}

const sensor_interface& device::get_sensor(size_t subdevice) const
{
    try
    {
        return *(_sensors.at(subdevice));
    }
    catch (std::out_of_range)
    {
        throw invalid_value_exception("invalid subdevice value");
    }
}

rs2_extrinsics device::get_extrinsics(size_t from_subdevice, rs2_stream, size_t to_subdevice, rs2_stream) const
{
    auto from = dynamic_cast<const sensor_base&>(get_sensor(from_subdevice)).get_pose(), 
         to   = dynamic_cast<const sensor_base&>(get_sensor(to_subdevice)).get_pose();
    if (from == to)
        return { {1,0,0,0,1,0,0,0,1}, {0,0,0} }; // identity transformation

    auto transform = inverse(from) * to;
    rs2_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

std::shared_ptr<matcher> librealsense::device::create_matcher(rs2_stream stream) const
{
    return std::make_shared<identity_matcher>( stream_id((device_interface*)(this), stream));
}
