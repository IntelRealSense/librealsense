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

size_t device::find_sensor_idx(const sensor_interface& s) const
{
    int idx = 0;
    for (auto&& sensor : _sensors)
    {
        if (&s == sensor.get()) return idx;
        idx++;
    }
    throw std::runtime_error("Sensor not found!");
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

void device::hardware_reset()
{
    throw not_implemented_exception(to_string() << __FUNCTION__ << " is not implemented for this device!");
}

std::shared_ptr<matcher> librealsense::device::create_matcher(const frame_holder& frame) const
{
    return std::make_shared<identity_matcher>( frame.frame->get_stream()->get_unique_id(), frame.frame->get_stream()->get_stream_type());
}
