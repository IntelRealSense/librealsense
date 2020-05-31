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
		virtual bool get_fw_log(fw_logs::fw_logs_binary_data& binary_data) = 0;
		virtual bool get_flash_log(fw_logs::fw_logs_binary_data& binary_data) = 0;
		virtual ~firmware_logger_extensions() = default;
	};
	MAP_EXTENSION(RS2_EXTENSION_FW_LOGGER, librealsense::firmware_logger_extensions);

	
	class firmware_logger_device : public virtual device, public firmware_logger_extensions
	{
	public:
		firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
			std::string camera_op_code);

		bool get_fw_log(fw_logs::fw_logs_binary_data& binary_data) override;
		bool get_flash_log(fw_logs::fw_logs_binary_data& binary_data) override;



	private:
		enum log_type
		{
			LOG_TYPE_FW_LOG = 0,
			LOG_TYPE_FLASH_LOG = 1,
			LOG_TYPE_NUM_OF_TYPES = 2
		};

		bool get_log(fw_logs::fw_logs_binary_data& binary_data, log_type type);
		void get_logs_from_hw_monitor(log_type type);

		std::vector<uint8_t> _input_code_for_fw_logs;
		std::vector<uint8_t> _input_code_for_flash_logs;
		std::shared_ptr<hw_monitor> _hw_monitor;

		std::queue<fw_logs::fw_logs_binary_data> _fw_logs_queue;
		std::queue<fw_logs::fw_logs_binary_data> _flash_logs_queue;



	};

}