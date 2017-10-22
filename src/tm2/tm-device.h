// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "device.h"

namespace librealsense
{
	class tm2_device : public virtual device
	{
	public:
		tm2_device(std::shared_ptr<context> ctx,
			const platform::backend_device_group& group);
	protected:

	private:

	};
}
