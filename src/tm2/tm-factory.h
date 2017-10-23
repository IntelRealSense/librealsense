// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "context.h"

#include <memory>
#include <vector>

namespace perc
{
	class TrackingManager;
	class TrackingDevice;
}

namespace librealsense
{
	class tm2_info : public device_info
	{
	public:
		std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx,
			bool register_device_notifications) const override;

		tm2_info(std::shared_ptr<perc::TrackingManager> manager,
			     perc::TrackingDevice* dev, 
			     std::shared_ptr<context> ctx,
				 platform::usb_device_info usb)
			: device_info(ctx), _usb(usb), _dev(dev), _manager(manager) {}

		static std::vector<std::shared_ptr<device_info>> pick_tm2_devices(
			std::shared_ptr<context> ctx,
			platform::backend_device_group& gproup);

		platform::backend_device_group get_device_data() const override
		{
			return platform::backend_device_group({}, { _usb }, {});
		}
	private:
		platform::usb_device_info _usb;
		std::shared_ptr<perc::TrackingManager> _manager;
		perc::TrackingDevice* _dev;
	};
}
