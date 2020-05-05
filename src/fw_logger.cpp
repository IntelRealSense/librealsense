// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "fw_logger.h"
#include <string>

namespace librealsense
{
	fw_logger::fw_logger(const char* camera_op_code)
	{
		auto op_code = static_cast<uint8_t>(std::stoi(camera_op_code));
		_input_code = { 0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
				 0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00 };
	
	}


	std::vector<uint8_t> fw_logger::get_fw_logs() const
	{
		return _debug_interface->send_receive_raw_data
	}
}