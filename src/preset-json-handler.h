// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <map>
#include <types.h>
#include "../third-party/json.hpp"

using json = nlohmann::json;

namespace librealsense
{
	class device_interface;

	namespace serializable_utilities
	{
		struct device_info
		{
			std::string name;
			std::string product_line;
			std::string fw_version;
		};

		class preset_json_reader
		{
		public:

            // C'tor may throw
			preset_json_reader( const std::string &json_content );
			bool check_device_info( const device_interface& device ) const;
			json::const_iterator find(const std::string& key) const;
			void ignore_device_info(const std::string& key);
			json::const_iterator end() const;
			json get_params() const { return *_parameters; };
				
		protected:
			device_info read_device_info() const;
			json get_value( json j, const std::string& field_key ) const;
			bool compare_device_info_field(const device_interface& device, const std::string& file_value, rs2_camera_info camera_info) const;
			bool validate_schema() const;
			device_info _device_info;
			std::string _schema_version;
			json _root;
			json *_parameters;
		};

        class preset_json_writer
        {
        public:
            preset_json_writer(); 

			void set_device_info(const device_interface& device);
			json get_params() const { return *_parameters; };
			json get_root() const { return _root; };

            template < typename T >
			void write_param(const std::string& key, T value)
			{
				(*_parameters)[key] = value;
			};

        protected:
			void write_schema();
            json _root;
            json *_parameters;
        };
	}
}
