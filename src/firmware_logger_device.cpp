// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"
#include <string>

namespace librealsense
{
	firmware_logger_device::firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
		std::string camera_op_code) :
		_hw_monitor(hardware_monitor)
	{
		auto op_code = static_cast<uint8_t>(std::stoi(camera_op_code.c_str()));
		_input_code = { 0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
				 0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00 };
	}


	std::vector<uint8_t> firmware_logger_device::get_fw_logs() const
	{
		auto res = _hw_monitor->send(_input_code);
		if (res.empty())
		{
			throw std::runtime_error("Getting Firmware logs failed!");
		}
		return res;
	}
}