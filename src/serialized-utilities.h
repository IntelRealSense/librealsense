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

	namespace serialized_utilities
	{
		struct device_info
		{
			std::string name;
			std::string product_line;
			std::string fw_version;
		};

		class json_preset_reader
		{
		public:

            // C'tor may throw
			json_preset_reader( const std::string &json_content );

			// Compares the device info fields on the preset json against the connected device.
			// if not match, it throws an informative exception
			void check_device_info( const device_interface& device ) const;

			// search for a key, if found it returns an iterator to it, works together with the end() function
			// example of use:
			// if (reader.find(key) != reader.end())
			//    {....}
			json::const_iterator find(const std::string& key) const;
			device_info get_device_info() const;

			// Allow override device info to allow ignoring some fields when checking device comparability.
			void override_device_info(const device_info& info);

			// for use together with find() function, same use as on STL containers.
			json::const_iterator end() const;

			// return only the parameters section
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

        class json_preset_writer
        {
        public:
            json_preset_writer(); 

			// sets and add a "device" section with the device information
			void set_device_info(const device_interface& device);

			// return only the parameters section
			json get_params() const { return *_parameters; };

			// return the root section (used to write all json to file)
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
