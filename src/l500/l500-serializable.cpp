//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l500-serializable.h"
#include "../../../third-party/json.hpp"
#include <set>
#include "l500-options.h"

namespace librealsense
{
    using json = nlohmann::json;

    l500_serializable::l500_serializable(std::shared_ptr<hw_monitor> hw_monitor, synthetic_sensor & depth_sensor)
        :_hw_monitor_ptr(hw_monitor),
         _depth_sensor(depth_sensor)
    {
    }

    std::vector<uint8_t> l500_serializable::serialize_json() const
    {
        return ivcam2::group_multiple_fw_calls( _depth_sensor, [&]() {

            json j;
            auto options = _depth_sensor.get_supported_options();

            for( auto o : options )
            {
                auto && opt = _depth_sensor.get_option( o );
                auto val = opt.query();
                j[get_string( o )] = val;
            }

            auto str = j.dump( 4 );
            return std::vector< uint8_t >( str.begin(), str.end() );
        } );
        
    }

    void l500_serializable::load_json( const std::string & json_content )
    {
        return ivcam2::group_multiple_fw_calls(_depth_sensor, [&]() {
            json j = json::parse( json_content );

            // Set of options that should not be set in the loop
            std::set< rs2_option > options_to_ignore{ RS2_OPTION_SENSOR_MODE,
                                                      RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH,
                                                      RS2_OPTION_RESET_CAMERA_ACCURACY_HEALTH };

            // We have to set the sensor mode (resolution) first
            auto & sensor_mode = _depth_sensor.get_option( RS2_OPTION_SENSOR_MODE );
            auto found_sensor_mode = j.find( get_string( RS2_OPTION_SENSOR_MODE ) );
            if( found_sensor_mode != j.end() )
            {
                float sensor_mode_val = found_sensor_mode.value();
                sensor_mode.set( sensor_mode_val );
            }

            // If a non custom preset is used, we should ignore all the settings that are
            // automatically set by the preset
            auto found_iterator = j.find( get_string( RS2_OPTION_VISUAL_PRESET ) );
            if( found_iterator != j.end() )
            {
                auto found_preset = rs2_l500_visual_preset( int( found_iterator.value() ) );
                if( found_preset != RS2_L500_VISUAL_PRESET_CUSTOM
                    && found_preset != RS2_L500_VISUAL_PRESET_DEFAULT )
                {
                    options_to_ignore.insert( RS2_OPTION_POST_PROCESSING_SHARPENING );
                    options_to_ignore.insert( RS2_OPTION_PRE_PROCESSING_SHARPENING );
                    options_to_ignore.insert( RS2_OPTION_NOISE_FILTERING );
                    options_to_ignore.insert( RS2_OPTION_AVALANCHE_PHOTO_DIODE );
                    options_to_ignore.insert( RS2_OPTION_CONFIDENCE_THRESHOLD );
                    options_to_ignore.insert( RS2_OPTION_LASER_POWER );
                    options_to_ignore.insert( RS2_OPTION_MIN_DISTANCE );
                    options_to_ignore.insert( RS2_OPTION_INVALIDATION_BYPASS );
                    options_to_ignore.insert( RS2_OPTION_DIGITAL_GAIN );
                }
            }

            auto opts = _depth_sensor.get_supported_options();

            auto default_preset = false;
            for( auto o : opts )
            {
                auto & opt = _depth_sensor.get_option( o );
                if( opt.is_read_only() )
                    continue;
                if( options_to_ignore.find( o ) != options_to_ignore.end() )
                    continue;

                auto key = get_string( o );
                auto it = j.find( key );
                if( it != j.end() )
                {
                    float val = it.value();
                    if( o == RS2_OPTION_VISUAL_PRESET
                        && (int)val == (int)RS2_L500_VISUAL_PRESET_DEFAULT )
                    {
                        default_preset = true;
                        continue;
                    }

                    opt.set( val );
                }
            }
            if( default_preset )
            {
                auto options = dynamic_cast< l500_options * >( this );
                if( options )
                {
                    auto preset = options->calc_preset_from_controls();
                    options->set_preset_value( preset );
                }
            }
        } );
    }
    } // namespace librealsense
