// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <preset-json-handler.h>
#include <types.h>
#include <device.h>

using json = nlohmann::json;

namespace librealsense {
namespace serializable_utilities {

preset_json_handler::preset_json_handler( const std::string & json_content )
{
    _presets_file = json::parse( json_content );
    _parameters = validate_schema() ? _presets_file.find( "parameters" ).value() : _presets_file;
    _header = read_header();
}

bool preset_json_handler::write_header()
{
   throw librealsense::invalid_value_exception("preset file was not loaded");
    // TODO continue

    return false;
}

preset_header preset_json_handler::read_header() const 
{
    preset_header header;

    header.schema_version = get_value( "", "schema version" );
    header.name = get_value( "device", "name" );
    header.product_line = get_value( "device", "product line" );
    header.fw_version = get_value( "device", "fw version" );
    return header;
}

bool preset_json_handler::check_device_compatibility( const device_interface & device ) const
{
    // Looks for device information
    auto file_device_it = _presets_file.find( "device" );
    if( file_device_it != _presets_file.end() )
    {
        if( ! _header.product_line.empty()
            && ! compare_header_field( device,
                                       _header.product_line,
                                       RS2_CAMERA_INFO_PRODUCT_LINE ) )
        {
            throw librealsense::invalid_value_exception(
                "preset product line does not match the connected device" );
        }

        if( ! _header.name.empty()
            && ! compare_header_field( device, _header.name, RS2_CAMERA_INFO_NAME ) )
        {
            throw librealsense::invalid_value_exception(
                "preset device name does not match the connected device" );
        }

        if( ! _header.fw_version.empty()
            && device.supports_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) )
        {
            // If we got a FW version but not a product name or FW version is smaller than the device FW version
            // --> Throw
            if(_header.product_line.empty() || firmware_version( _header.fw_version )
                < firmware_version( device.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) ) )
                throw librealsense::invalid_value_exception(
                    "preset device FW version is lower than the connected device FW version" );
        }
    }

    return true;
}

std::string preset_json_handler::get_value(const std::string& parent_key, const std::string& field_key) const
{
    if (!parent_key.empty())
    {
        auto parent_it = _presets_file.find( parent_key );
        if (parent_it != _presets_file.end())
        {
            auto it = (*parent_it).find( field_key );
            if (it != (*parent_it).end())
            {
                return it.value().get< std::string >();
            }
        }
    }
    else
    {
        auto it = _presets_file.find( field_key );
        if (it != _presets_file.end())
        {
            return it.value().get< std::string >();
        }
    }
   
    return "";
}

bool preset_json_handler::compare_header_field(const device_interface& device, const std::string& file_value, rs2_camera_info camera_info) const
{
    if (device.supports_info(camera_info))
    {
        return file_value == device.get_info(camera_info);
    }
    return false;
}

bool librealsense::serializable_utilities::preset_json_handler::validate_schema() const
{
    auto schema_version_found = _presets_file.find("schema version") != _presets_file.end();
    auto device_found = _presets_file.find("device") != _presets_file.end();
    auto parameters_found = _presets_file.find("parameters") != _presets_file.end();

    if (schema_version_found && device_found && parameters_found) return true;
    if (!schema_version_found && !device_found && !parameters_found) return false;

    throw librealsense::invalid_value_exception("preset file is corrupted , cannot validate schema.");
}

json::const_iterator preset_json_handler::find(const std::string& key) const
{
    return _parameters.find(key);
}

json::const_iterator preset_json_handler::end() const
{
    return _parameters.end();
}

void preset_json_handler::ignore_device_info( const std::string& key)
{
    auto dev_it = _presets_file.find("device");

    if (dev_it != _presets_file.end())
    {
        dev_it->erase(key);
    }
}

}  // namespace serializable_utilities
}  // namespace librealsense