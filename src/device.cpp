// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "device.h"
#include "image.h"

using namespace rsimpl;

device::device()
{
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

int device::add_endpoint(std::shared_ptr<endpoint> endpoint)
{
    _endpoints.push_back(std::move(endpoint));
    return _endpoints.size() - 1;
}

uvc_endpoint& device::get_uvc_endpoint(int sub)
{
    return static_cast<uvc_endpoint&>(*_endpoints[sub]);
}

void device::register_device(std::string name, std::string fw_version, std::string serial, std::string location)
{
    _static_info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = std::move(name);
    _static_info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = std::move(fw_version);
    _static_info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = std::move(serial);
    _static_info.camera_info[RS_CAMERA_INFO_DEVICE_LOCATION] = std::move(location);
}

void device::declare_capability(supported_capability cap)
{
    _static_info.capabilities_vector.push_back(cap);
}

rs_extrinsics device::get_extrinsics(int from_subdevice, int to_subdevice)
{
    auto from = get_pose(from_subdevice), to = get_pose(to_subdevice);
    if (from == to) return { {1,0,0,0,1,0,0,0,1}, {0,0,0} }; // identity transformation
    auto transform = inverse(from) * to;
    rs_extrinsics extrin;
    (float3x3 &)extrin.rotation = transform.orientation;
    (float3 &)extrin.translation = transform.position;
    return extrin;
}

void device::set_depth_scale(float scale)
{
    _static_info.nominal_depth_scale = scale;
}
