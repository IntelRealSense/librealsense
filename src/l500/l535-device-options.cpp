//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-device-options.h"
#include "l535-amc-option.h"
#include "l535-preset-option.h"
#include "l500-private.h"
#include "l500-depth.h"

using librealsense::ivcam2::l535::device_options;

device_options::device_options( std::shared_ptr< librealsense::context > ctx,
                                const librealsense::platform::backend_device_group & group )
    : device( ctx, group )
    , l500_device( ctx, group )
{
    auto & raw_depth_sensor = get_raw_depth_sensor();
    auto & depth_sensor = get_depth_sensor();

    // Keep the USB power on while triggering multiple HW monitor commands on it.
    ivcam2::group_multiple_fw_calls( depth_sensor, [&]() {
        auto default_sensor_mode = RS2_SENSOR_MODE_VGA;

        std::map< rs2_option, std::pair< amc_control, std::string > > options = {
            { RS2_OPTION_POST_PROCESSING_SHARPENING,
              { post_processing_sharpness,
                "Changes the amount of sharpening in the post-processed image" } },
            { RS2_OPTION_PRE_PROCESSING_SHARPENING,
              { pre_processing_sharpness,
                "Changes the amount of sharpening in the pre-processed image" } },
            { RS2_OPTION_NOISE_FILTERING,
              { noise_filtering, "Control edges and background noise" } },
            { RS2_OPTION_AVALANCHE_PHOTO_DIODE,
              { apd, "Changes the exposure time of Avalanche Photo Diode in the receiver" } },
            { RS2_OPTION_CONFIDENCE_THRESHOLD,
              { confidence,
                "The confidence level threshold to use to mark a pixel as valid by the depth "
                "algorithm" } },
            { RS2_OPTION_LASER_POWER,
              { laser_gain, "Power of the laser emitter, with 0 meaning projector off" } },
            { RS2_OPTION_MIN_DISTANCE,
              { min_distance, "Minimal distance to the target (in mm)" } },
            { RS2_OPTION_INVALIDATION_BYPASS,
              { invalidation_bypass, "Enable/disable pixel invalidation" } },
             { RS2_OPTION_ALTERNATE_IR,
              { alternate_ir, "Enable/Disable alternate IR" } },
            { RS2_OPTION_AUTO_RX_SENSITIVITY, 
              { auto_rx_sensitivity, "Enable receiver sensitivity according to ambient light, bounded by the Receiver Gain control" } },
            { RS2_OPTION_TRANSMITTER_FREQUENCY,
              { transmitter_frequency, "Change transmitter frequency, increasing effective range over sharpness" } }
        };

        for( auto i : options )
        {
            auto opt = std::make_shared< amc_option >( this,
                                                       _hw_monitor.get(),
                                                       i.second.first,
                                                       i.second.second );

            depth_sensor.register_option( i.first, opt );
            _advanced_options.push_back( i.first );
        }

        auto preset = std::make_shared< preset_option >(
            option_range{ RS2_L500_VISUAL_PRESET_CUSTOM,
                          RS2_L500_VISUAL_PRESET_CUSTOM,
                          1,
                          RS2_L500_VISUAL_PRESET_CUSTOM },
            "Preset to calibrate the camera" ); //todo:improve the 

        depth_sensor.register_option( RS2_OPTION_VISUAL_PRESET, preset );
    } );
}
