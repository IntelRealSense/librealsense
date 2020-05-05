// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

namespace librealsense
{
	class fw_logger
	{
	public:
		fw_logger(const char* camera_op_code);

		std::vector<uint8_t> get_fw_logs() const;

	private:
		std::vector<uint8_t> _input_code;
	};

}