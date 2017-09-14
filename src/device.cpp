// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"
#include "stream.h"
#include "environment.h"

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

std::pair<uint32_t, rs2_extrinsics> librealsense::device::get_extrinsics(const stream_interface& stream) const
{
    auto stream_index = stream.get_unique_id();
    auto pair = _extrinsics.at(stream_index);
    auto pin_stream = pair.second;
    rs2_extrinsics ext{};
    if (environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(*pin_stream, stream, &ext) == false)
    {
        throw std::runtime_error(to_string() << "Failed to fetch extrinsics between pin stream (" << pin_stream->get_unique_id() << ") to given stream (" << stream.get_unique_id() << ")");
    }
    return std::make_pair(pair.first, ext);
}

void librealsense::device::register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t groupd_index)
{
    auto iter = std::find_if(_extrinsics.begin(), 
                           _extrinsics.end(),
                           [groupd_index](const std::pair<int, std::pair<uint32_t, std::shared_ptr<const stream_interface>>>& p) { return p.second.first == groupd_index; });
    if (iter == _extrinsics.end())
    {
        //First stream to register for this group
        _extrinsics[stream.get_unique_id()] = std::make_pair(groupd_index, stream.shared_from_this());
    }
    else
    {
        //iter->second holds the group_id and the key stream
        _extrinsics[stream.get_unique_id()] = iter->second;
    }
}
