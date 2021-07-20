// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <preset-json-handler.h>
#include <types.h>
#include <device.h>

using json = nlohmann::json;

namespace librealsense {
namespace serializable_utilities {

preset_json_handler::preset_json_handler( const device_interface & device,
                                          const std::string & json_content )
    : _device( device )
    , _status( status::UNLOADED )
{
    _presets_file = json::parse( json_content );
    _status = verify_header() ? status::LOADED_SCHEME_FORMAT : status::LOADED_FLAT_FORMAT;
}

bool preset_json_handler::write_header()
{
    if ( _status == status::UNLOADED ) throw librealsense::invalid_value_exception("preset file was not loaded");
    // TODO continue

    return false;
}

bool preset_json_handler::verify_header() const
{
    static std::map< const std::string, rs2_camera_info > device_info_to_option_map
        = { { "name", RS2_CAMERA_INFO_NAME },
            { "product line", RS2_CAMERA_INFO_PRODUCT_LINE },
            { "fw version", RS2_CAMERA_INFO_FIRMWARE_VERSION } };

    // Old style format,
    // TODO add a hard coded scheme version!
    if( _presets_file.find( "schema version" ) == _presets_file.end()
        || _presets_file.find( "device" ) == _presets_file.end()
        || _presets_file.find( "parameters" ) == _presets_file.end() )
        return false;

    // Looks for device information
    auto &file_device_info = _presets_file.find("device");
    if( file_device_info != _presets_file.end() )
    {
        // device section found, iterate it and verify device info against connected device info
        for( auto file_device_info_it = file_device_info->begin();
             file_device_info_it != file_device_info->end();
             ++file_device_info_it )
        {
            auto device_info_to_option = device_info_to_option_map.find( file_device_info_it.key() );
            if(device_info_to_option != device_info_to_option_map.end() )
            {
                    if( _device.supports_info(device_info_to_option->second ) )
                    {
                        if (device_info_to_option->first == "fw version")
                        {
                            auto file_fw_version = file_device_info_it.value().get< std::string >();
                            if (firmware_version(file_fw_version) < firmware_version(_device.get_info(device_info_to_option->second)))
                            {
                                throw librealsense::invalid_value_exception(
                                    "preset file FW version is lower then connected device FW version");
                            }
                        }
                        else if( _device.get_info(device_info_to_option->second ) != file_device_info_it.value().get< std::string >() )
                            throw librealsense::invalid_value_exception(
                                "preset file header do not match connected device" );
                    }
                
            }
        }
    }
    return true;
}

}  // namespace serializable_utilities
}  // namespace librealsense