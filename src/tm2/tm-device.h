// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "device.h"

#include "TrackingManager.h"

#include <memory>
#include <vector>

namespace librealsense
{
	class tm2_device : public virtual device
	{
	public:
		tm2_device(std::shared_ptr<perc::TrackingManager> manager,
			perc::TrackingDevice* dev,
			std::shared_ptr<context> ctx,
			const platform::backend_device_group& group);

	private:
		std::shared_ptr<perc::TrackingManager> _manager;
		perc::TrackingDevice* _dev;
	};
}
