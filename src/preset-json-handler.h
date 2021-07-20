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
		class preset_json_handler
		{
		public:
            // C'tor may throw
			preset_json_handler(const device_interface& device, const std::string &json_content);

			bool write_header();
			json::const_iterator find(const std::string& key) const;

            json::const_iterator end() const
            {
                return _presets_file.end();
            }

				
		protected:
			bool verify_header() const;

			const device_interface& _device;
			json _presets_file;
			enum class status {UNLOADED, LOADED_FLAT_FORMAT, LOADED_SCHEME_FORMAT};
			status _status;
		};
	}
}
