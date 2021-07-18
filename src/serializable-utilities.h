// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>

namespace librealsense
{
	class device_interface;

	namespace serializable_utilities
	{
		struct serialized_device_info
		{
			std::string device_name;
		};

		std::string get_connected_device_info( device_interface &sensor );
		serialized_device_info get_pre_configured_device_info( const librealsense::device_interface& device, const std::string& json_content);
	}
}
