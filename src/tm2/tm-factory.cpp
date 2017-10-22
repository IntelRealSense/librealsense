// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "tm-factory.h"

namespace librealsense
{
	std::shared_ptr<device_interface> tm2_info::create(std::shared_ptr<context> ctx,
		bool register_device_notifications) const
	{
		return nullptr;
	}

	std::vector<std::shared_ptr<device_info>> tm2_info::pick_tm2_devices(
		std::shared_ptr<context> ctx,
		platform::backend_device_group& group)
	{
		std::vector<std::shared_ptr<device_info>> results;
		return results;
	}
}
