// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"
#include <chrono>
#include <memory>
#include <vector>
#include "hw-monitor.h"
#include "option.h"


namespace rsimpl
{
    class device
    {
    public:
        device();

        virtual ~device() = default;

        bool supports(rs_subdevice subdevice) const;

        endpoint& get_endpoint(rs_subdevice sub) { return *_endpoints[sub]; }

        option& get_option(rs_subdevice subdevice, rs_option id);

        bool supports_option(rs_subdevice subdevice, rs_option id);

        const std::string& get_info(rs_camera_info info) const;

        bool supports_info(rs_camera_info info) const;

        rs_extrinsics get_extrinsics(rs_subdevice from, rs_subdevice to);

        virtual rs_intrinsics get_intrinsics(rs_subdevice subdevice, stream_request profile) const { throw std::exception("Not Implemented"); };

        float get_depth_scale() const { return _static_info.nominal_depth_scale; }

    protected:
        void assign_endpoint(rs_subdevice subdevice,
                             std::shared_ptr<endpoint> endpoint);

        uvc_endpoint& get_uvc_endpoint(rs_subdevice sub);

        void register_option(rs_option id, rs_subdevice subdevice, std::shared_ptr<option> option);

        void register_pu(rs_subdevice subdevice, rs_option id);

        void register_device(std::string name, std::string fw_version, std::string serial, std::string location);

        void set_pose(rs_subdevice subdevice, pose p);

        pose get_pose(rs_subdevice subdevice) const;

        void declare_capability(supported_capability cap);

        void set_depth_scale(float scale);

    private:
        std::vector<std::shared_ptr<endpoint>> _endpoints;
        std::map<std::pair<rs_subdevice, rs_option>, std::shared_ptr<option>> _options;
        static_device_info _static_info;
    };
}
