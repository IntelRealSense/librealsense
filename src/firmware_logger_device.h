// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"
#include "device.h"
#include <vector>
#include "fw-logs/fw-log-data.h"

namespace librealsense
{
	class firmware_logger_extensions
	{
	public:
		virtual std::vector<uint8_t> get_fw_binary_logs() const = 0; 
		virtual std::vector<fw_logs::fw_log_data> get_fw_logs() = 0;
		virtual std::vector<rs2_firmware_log_message> get_fw_logs_msg() = 0;
		virtual ~firmware_logger_extensions() = default;
	};
	MAP_EXTENSION(RS2_EXTENSION_FW_LOGGER, librealsense::firmware_logger_extensions);

	
	class firmware_logger_device : public virtual device, public firmware_logger_extensions
	{
	public:
		firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
			std::string camera_op_code);

		std::vector<uint8_t> get_fw_binary_logs() const override;
		std::vector<fw_logs::fw_log_data> get_fw_logs() override;
		std::vector<rs2_firmware_log_message> get_fw_logs_msg() override;
		fw_logs::fw_log_data fill_log_data(const char* fw_logs) ;

	private:
		std::vector<uint8_t> _input_code;
		std::shared_ptr<hw_monitor> _hw_monitor;
		uint64_t _last_timestamp;
		const double _timestamp_factor;
	};

}