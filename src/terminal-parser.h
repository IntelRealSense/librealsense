// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"
#include "device.h"
#include <vector>
#include "../common/parser.hpp"

namespace librealsense
{
	class terminal_parser_interface
	{
	public:
		//virtual bool get_fw_log(fw_logs::fw_logs_binary_data& binary_data) = 0;

		virtual ~terminal_parser_interface() = default;
	};


	class terminal_parser : public terminal_parser_interface
	{
	public:
		terminal_parser(const std::string& xml_full_file_path);

		std::vector<uint8_t> parse_command(debug_interface* device, const std::string& command);

	private:
		debug_interface* _device;
		std::map<std::string, xml_parser_function> _format_type_to_lambda;
		commands_xml _cmd_xml;

		std::vector<uint8_t> build_raw_command_data(const command_from_xml& command,
			const std::vector<std::string>& params);	

	};

}