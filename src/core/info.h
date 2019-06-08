// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "extension.h"
#include <string>

namespace librealsense
{
    class info_interface : public virtual recordable<info_interface>
    {
    public:
        virtual const std::string& get_info(rs2_camera_info info) const = 0;
        virtual bool supports_info(rs2_camera_info info) const = 0;

        virtual ~info_interface() = default;
    };

    MAP_EXTENSION(RS2_EXTENSION_INFO, librealsense::info_interface);

    class LRS_EXTENSION_API info_container : public virtual info_interface, public extension_snapshot
    {
    public:
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;

        void register_info(rs2_camera_info info, const std::string& val);
        void update_info(rs2_camera_info info, const std::string& val);
        void create_snapshot(std::shared_ptr<info_interface>& snapshot) const override;
        void enable_recording(std::function<void(const info_interface&)> record_action) override;
        void update(std::shared_ptr<extension_snapshot> ext) override;
    private:
        std::map<rs2_camera_info, std::string> _camera_info;
    };

}
