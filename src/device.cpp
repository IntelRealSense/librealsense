// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "hw-monitor.h"
#include "image.h"

#include <array>
#include <algorithm>
#include <sstream>

using namespace rsimpl;

device::device()
{
    _endpoints.resize(RS_SUBDEVICE_COUNT);
}

bool device::supports(rs_subdevice subdevice) const
{
    return _endpoints[subdevice].get() != nullptr;
}

option& device::get_option(rs_subdevice subdevice, rs_option id)
{
    auto it = _options.find(std::make_pair(subdevice, id));
    if (it == _options.end())
    {
        throw std::runtime_error(to_string() 
            << "Subdevice " << rs_subdevice_to_string(subdevice) 
            << " does not support option " 
            << rs_option_to_string(id) << "!");
    }
    return *it->second;
}

bool device::supports_option(rs_subdevice subdevice, rs_option id)
{
    auto it = _options.find(std::make_pair(subdevice, id));
    if (it == _options.end()) return false;
    return it->second->is_enabled();
}

const std::string& device::get_info(rs_camera_info info) const
{
    auto it = _static_info.camera_info.find(info);
    if (it == _static_info.camera_info.end())
        throw std::runtime_error("Selected camera info is not supported for this camera!");
    return it->second;
}

bool device::supports_info(rs_camera_info info) const
{
    auto it = _static_info.camera_info.find(info);
    return it != _static_info.camera_info.end();
}

void device::assign_endpoint(rs_subdevice subdevice, std::shared_ptr<endpoint> endpoint)
{
    _endpoints[subdevice] = std::move(endpoint);
}

uvc_endpoint& device::get_uvc_endpoint(rs_subdevice sub)
{
    return static_cast<uvc_endpoint&>(*_endpoints[sub]);
}

void device::register_option(rs_option id, rs_subdevice subdevice, std::shared_ptr<option> option)
{
    _options[std::make_pair(subdevice, id)] = std::move(option);
}

void device::register_pu(rs_subdevice subdevice, rs_option id)
{
    register_option(id, subdevice, std::make_shared<uvc_pu_option>(get_uvc_endpoint(subdevice), id));
}

void device::register_device(std::string name, std::string fw_version, std::string serial, std::string location)
{
    _static_info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = std::move(name);
    _static_info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = std::move(fw_version);
    _static_info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = std::move(serial);
    _static_info.camera_info[RS_CAMERA_INFO_DEVICE_LOCATION] = std::move(location);
}

void device::set_pose(rs_subdevice subdevice, pose p)
{
    _static_info.subdevice_poses[subdevice] = p;
}

void device::declare_capability(supported_capability cap)
{
    _static_info.capabilities_vector.push_back(cap);
}

void device::set_depth_scale(float scale)
{
    _static_info.nominal_depth_scale = scale;
}
