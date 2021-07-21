// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "serialized-utilities.h"
#include <types.h>
#include <device.h>

using json = nlohmann::json;

namespace librealsense {
namespace serialized_utilities {

static const char* SCHEMA_VERSION = "1.0";

json_preset_reader::json_preset_reader( const std::string & json_content ) : _parameters(nullptr)
{
    _root = json::parse( json_content );
    if (validate_schema())
    {
        _schema_version = get_value(_root, "schema version").get<std::string>();
        _device_info = read_device_info();
        _parameters = &_root["parameters"];
    }
    else
    {
        _parameters = &_root;
    }
}

device_info json_preset_reader::read_device_info() const
{
    device_info info;
    auto device = get_value( _root, "device" );
    if (!device.is_null())
    {
        auto device_name = get_value(device, "name");
        info.name = device_name.is_null() ? "" : device_name.get< std::string >();

        auto product_line = get_value(device, "product line");
        info.product_line = product_line.is_null() ? "" : product_line.get< std::string >();

        auto fw_version = get_value(device, "fw version");
        info.fw_version = fw_version.is_null() ? "" : fw_version.get< std::string >();
    }
    return info;
}

bool json_preset_reader::check_device_info( const device_interface & device ) const
{
    // Looks for device information
    if( ! _device_info.product_line.empty()
        && ! compare_device_info_field( device,
                                        _device_info.product_line,
                                        RS2_CAMERA_INFO_PRODUCT_LINE ) )
    {
        throw librealsense::invalid_value_exception(
            "preset product line does not match the connected device" );
    }

    if( ! _device_info.name.empty()
        && ! compare_device_info_field( device, _device_info.name, RS2_CAMERA_INFO_NAME ) )
    {
        throw librealsense::invalid_value_exception(
            "preset device name does not match the connected device" );
    }

    if( ! _device_info.fw_version.empty()
        && device.supports_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) )
    {
        // If we got a FW version but not a product name or FW version is smaller than the device FW
        // version
        // --> Throw
        if( _device_info.product_line.empty()
            || firmware_version( _device_info.fw_version )
                   < firmware_version( device.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION ) ) )
            throw librealsense::invalid_value_exception(
                "preset device FW version is lower than the connected device FW version" );
    }

    return true;
}

json json_preset_reader::get_value(json j, const std::string& field_key) const
{
    auto val_it = j.find(field_key);
    if (val_it != j.end())
    {
        return val_it.value();
    }
    
    return json();
}

bool json_preset_reader::compare_device_info_field(const device_interface& device, const std::string& file_value, rs2_camera_info camera_info) const
{
    if (device.supports_info(camera_info))
    {
        return file_value == device.get_info(camera_info);
    }
    return false;
}

bool json_preset_reader::validate_schema() const
{
    auto schema_version_found = _root.find("schema version") != _root.end();
    //auto device_found = _root.find("device") != _root.end();
    auto parameters_found = _root.find("parameters") != _root.end();

    if (schema_version_found &&/* device_found &&*/ parameters_found) return true;
    if (!schema_version_found && /*!device_found &&*/ !parameters_found) return false;

    throw librealsense::invalid_value_exception("preset file is corrupted , cannot validate schema.");
}

json::const_iterator json_preset_reader::find(const std::string& key) const
{
    return _parameters->find(key);
}

json::const_iterator json_preset_reader::end() const
{
    return _parameters->end();
}

void json_preset_reader::ignore_device_info( const std::string& key)
{
    auto dev_it = _root.find("device");
    if (dev_it != _root.end())
    {
        dev_it->erase(key);
    }
}

json_preset_writer::json_preset_writer() : _parameters(nullptr)
{
    write_schema();
    _parameters = &_root["parameters"];
}

void json_preset_writer::set_device_info(const device_interface& device)
{
    _root["device"] = json::object();

    auto& device_section = _root["device"];

    for (auto&& val : std::map<std::string, rs2_camera_info>{ { "name", RS2_CAMERA_INFO_NAME }, { "product line", RS2_CAMERA_INFO_PRODUCT_LINE }, { "fw version", RS2_CAMERA_INFO_FIRMWARE_VERSION } })
    {
        if (device.supports_info(val.second))
        {
            device_section[val.first] = device.get_info(val.second);
        }
    }
}

void json_preset_writer::write_schema()
{
    _root["schema version"] = SCHEMA_VERSION;
    _root["parameters"] = json::object();
}

}  // namespace librealsense
}  // namespace serialized_utilities