// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <map>
#include <types.h>
#include "../third-party/json.hpp"

using json = nlohmann::json;




/// <summary>
/// check device compatibility
/// Ctor just save header and parameters iterator (need to save the end itor as well
/// D400 will ignore the camera name
/// </summary>
/// 
/// 
namespace librealsense
{
	class device_interface;

	namespace serializable_utilities
	{
		struct preset_header
		{
			std::string schema_version;
			std::string name;
			std::string product_line;
			std::string fw_version;
		};

		class preset_json_handler
		{
		public:

            // C'tor may throw
			preset_json_handler( const std::string &json_content );
			bool check_device_compatibility( const device_interface& device ) const;
			bool write_header();
			json::const_iterator find(const std::string& key) const;
			void ignore_device_info(const std::string& key);
			json::const_iterator end() const;

				
		protected:
			preset_header read_header() const; 
			std::string get_value( const std::string& parent_key, const std::string& field_key ) const;
			bool compare_header_field(const device_interface& device, const std::string& file_value, rs2_camera_info camera_info) const;
			bool validate_schema() const;
			preset_header _header;
			json _presets_file;
			json _parameters;
			json::iterator _parameters_end;
		};
	}
}
