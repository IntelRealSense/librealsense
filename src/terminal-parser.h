// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"
#include "device.h"
#include <vector>
#include "../common/parser.hpp"

namespace librealsense
{
	class terminal_parser
	{
	public:
		terminal_parser(const std::string& xml_full_file_path);


		std::vector<uint8_t> parse_command(const std::string& command);

		std::vector<uint8_t> parse_response(const std::string& command, 
			const std::vector<uint8_t>& response);

	private:
		void get_command_and_params_from_input(const std::string& line, command_from_xml& command,
			std::vector<std::string>& params);
		std::vector<uint8_t> build_raw_command_data(const command_from_xml& command,
			const std::vector<std::string>& params);	

		std::map<std::string, xml_parser_function> _format_type_to_lambda;
		commands_xml _cmd_xml;
	};

}