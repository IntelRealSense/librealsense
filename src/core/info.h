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

    class info_container : public virtual info_interface
    {
    public:
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;

        void register_info(rs2_camera_info info, const std::string& val);
        void create_snapshot(std::shared_ptr<info_interface>& snapshot) override;
        void create_recordable(std::shared_ptr<info_interface>& recordable,
            std::function<void(std::shared_ptr<extension_snapshot>)> record_action) override;

    private:
        std::map<rs2_camera_info, std::string> _camera_info;
    };

    class info_snapshot : public extension_snapshot_base<info_interface>, public info_container
    {
    public:
        info_snapshot(const std::map<rs2_camera_info, std::string>& values)
        {
            for (auto value : values)
            {
                register_info(value.first, value.second);
            }
        }
        info_snapshot(info_interface* info_api)
        {
            update_self(info_api);
        }
    private:
        void update_self(info_interface* info_api)
        {
            for (int i = 0; i < RS2_CAMERA_INFO_COUNT; ++i)
            {
                rs2_camera_info info = static_cast<rs2_camera_info>(i);
                if (info_api->supports_info(info))
                {
                    register_info(info, info_api->get_info(info));
                }
            }
        }
    };
}
