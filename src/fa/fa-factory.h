// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#pragma once

#include "fa-private.h"

namespace librealsense
{
    class fa_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx,
                                                 bool register_device_notifications) const override;

        explicit fa_info(std::shared_ptr<context> ctx,
            std::vector<platform::uvc_device_info> uvcs)
            : device_info(ctx), _uvcs(std::move(uvcs)) {}
			
		static std::vector<std::shared_ptr<device_info>> pick_uvc_devices(
            const std::shared_ptr<context>& ctx,
            const std::vector<platform::uvc_device_info>& uvc_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            auto groups = group_devices_by_unique_id(uvc_devices);

            for (auto&& g : groups)
            {
                if (g.front().vid != VID_INTEL_CAMERA)
                    list.push_back(std::make_shared<fa_info>(ctx, g));
            }
            return list;
        }

        static std::vector<std::shared_ptr<device_info>> pick_fa_devices(
            std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& uvc_devices);

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group(_uvcs);
        }

    private:
        std::vector<platform::uvc_device_info> _uvcs;
    };
}
