// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "rsdds-serializable.h"

#include <src/serialized-utilities.h>
#include <src/core/sensor-interface.h>
#include <src/core/device-interface.h>

#include <rsutils/json.h>
#include <set>

using json = nlohmann::json;


static std::set< rs2_option > get_options_to_ignore()
{
    return {
        RS2_OPTION_FRAMES_QUEUE_SIZE,  // Internally added and is not an option we need to record/load
        RS2_OPTION_REGION_OF_INTEREST  // The RoI is temporary, uses another mechanism for get/set, and we don't load it
    };
}


namespace librealsense {


std::vector< uint8_t > dds_serializable::serialize_json() const
{
    serialized_utilities::json_preset_writer writer;
    writer.set_device_info( get_serializable_device() );

    // Set of options that should not be written
    auto const options_to_ignore = get_options_to_ignore();

    for( auto p_sensor : get_serializable_sensors() )
    {
        std::string const sensor_name = p_sensor->get_info( RS2_CAMERA_INFO_NAME );
        for( auto o : p_sensor->get_supported_options() )
        {
            auto & opt = p_sensor->get_option( o );
            if( opt.is_read_only() )
                continue;
            if( options_to_ignore.find( o ) != options_to_ignore.end() )
                continue;

            auto val = opt.get_value();
            writer.write_param( sensor_name + '/' + get_string( o ), val );
        }
    }

    auto str = writer.to_string();
    return std::vector< uint8_t >( str.begin(), str.end() );
}


void dds_serializable::load_json( const std::string & json_content )
{
    serialized_utilities::json_preset_reader reader( json_content );

    // Verify if device information in preset file is compatible with the connected device.
    reader.check_device_info( get_serializable_device() );
    auto const sensors = get_serializable_sensors();

    // Do the visual preset first, as it sets the basis on top of which we apply the regular controls
    {
        auto const visual_preset_subkey = '/' + get_string( RS2_OPTION_VISUAL_PRESET );
        for( auto const p_sensor : sensors )
        {
            std::string const sensor_name = p_sensor->get_info( RS2_CAMERA_INFO_NAME );
            auto const key = sensor_name + visual_preset_subkey;
            auto it = reader.find( key );
            if( it == reader.end() )
                continue;

            try
            {
                auto & opt = p_sensor->get_option( RS2_OPTION_VISUAL_PRESET );
                opt.set_value( it.value() );
                LOG_DEBUG( "loaded '" << key << "' value " << it.value() );
            }
            catch( std::exception const & e )
            {
                LOG_ERROR( "Failed to load '" << key << "' (value " << it.value() << "): " << e.what() );
                throw;
            }
        }
    }

    // Create a set of options we have to change
    std::map< std::string /*key*/, std::string /*error*/ > keys_to_change;
    {
        // Set of options that should not be set in the loop
        auto options_to_ignore = get_options_to_ignore();
        options_to_ignore.insert( RS2_OPTION_VISUAL_PRESET );  // We don't want to do this again

        for( auto const p_sensor : sensors )
        {
            std::string const sensor_name = p_sensor->get_info( RS2_CAMERA_INFO_NAME );
            for( auto option_id : p_sensor->get_supported_options() )
            {
                if( options_to_ignore.find( option_id ) != options_to_ignore.end() )
                    continue;
                auto key = sensor_name + '/' + get_string( option_id );
                if( reader.find( key ) == reader.end() )
                    continue;

                keys_to_change.emplace( std::move( key ), std::string() );
            }
        }
    }

    // Some options may depend on others being set first (e.g., they are read-only until another option is set); these
    // will generate errors. Since we don't know which order to set options in, we track failures and keep trying as
    // long as at least one option value is changed:
    while( true )
    {
        std::map< std::string /*key*/, std::string /*error*/ > keys_left;
        for( auto const p_sensor : sensors )
        {
            std::string const sensor_name = p_sensor->get_info( RS2_CAMERA_INFO_NAME );
            for( auto option_id : p_sensor->get_supported_options() )
            {
                auto key = sensor_name + '/' + get_string( option_id );
                if( keys_to_change.find( key ) == keys_to_change.end() )
                    continue;

                auto it = reader.find( key );
                if( it != reader.end() )
                {
                    try
                    {
                        auto & opt = p_sensor->get_option( option_id );
                        opt.set_value( it.value() );
                        LOG_DEBUG( "loaded '" << key << "' value " << it.value() );
                        continue;
                    }
                    catch( unrecoverable_exception const & )
                    {
                        throw;
                    }
                    catch( std::exception const & e )
                    {
                        std::string error = e.what();
                        LOG_DEBUG( "Failed to load '" << key << "' (value " << it.value() << "): " << error );
                        keys_left.emplace( std::move( key ), std::move( error ) );
                    }
                }
            }
        }
        if( keys_left.empty() )
            // Nothing more to do; we can stop
            break;
        if( keys_left.size() == keys_to_change.size() )
        {
            // We still have failures, but nothing's changed; we must stop
            for( auto const & key_error : keys_left )
            {
                auto it = reader.find( key_error.first );
                if( it != reader.end() )
                    LOG_ERROR( "Failed to load '" << key_error.first << "' (value " << it.value() << "): " << key_error.second );
            }
            break;
        }
        keys_to_change = std::move( keys_left );
    }
}


}  // namespace librealsense
