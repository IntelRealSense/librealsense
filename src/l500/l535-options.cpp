//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "l535-options.h"
#include "l535-amc-option.h"
#include "l535-preset-option.h"
#include "l500-private.h"
#include "l500-depth.h"

namespace librealsense
{
    namespace ivcam2
    {
        namespace l535
        {
            l535_options::l535_options(std::shared_ptr< context > ctx,
                const platform::backend_device_group & group)
                : device(ctx, group)
                , l500_device(ctx, group)
            {
                auto & raw_depth_sensor = get_raw_depth_sensor();
                auto & depth_sensor = get_depth_sensor();

                // Keep the USB power on while triggering multiple HW monitor commands on it.
                ivcam2::group_multiple_fw_calls(depth_sensor, [&]() {
                    auto default_sensor_mode = RS2_SENSOR_MODE_VGA;

                    std::map< rs2_option, std::pair< amc_control, std::string > > options = {
                        { RS2_OPTION_POST_PROCESSING_SHARPENING,
                          { l535_post_processing_sharpness, "Changes the amount of sharpening in the post-processed image" } },
                        { RS2_OPTION_PRE_PROCESSING_SHARPENING,
                          { l535_pre_processing_sharpness, "Changes the amount of sharpening in the pre-processed image" } },
                        { RS2_OPTION_NOISE_FILTERING,
                          { l535_noise_filtering, "Control edges and background noise" } },
                        { RS2_OPTION_AVALANCHE_PHOTO_DIODE,
                          { l535_apd, "Changes the exposure time of Avalanche Photo Diode in the receiver" } },
                        { RS2_OPTION_CONFIDENCE_THRESHOLD,
                          { l535_confidence, "The confidence level threshold to use to mark a pixel as valid by the depth algorithm" } },
                        { RS2_OPTION_LASER_POWER,
                          { l535_laser_gain, "Power of the laser emitter, with 0 meaning projector off" } },
                        { RS2_OPTION_MIN_DISTANCE,
                          { l535_min_distance, "Minimal distance to the target (in mm)" } },
                        { RS2_OPTION_INVALIDATION_BYPASS,
                          { l535_invalidation_bypass, "Enable/disable pixel invalidation" } },
                        { RS2_OPTION_INVALIDATION_BYPASS,
                          { l535_invalidation_bypass, "Enable/disable pixel invalidation" } },
                        {RS2_OPTION_RX_SENSITIVITY,{ l535_rx_sensitivity, "auto gain"}} //TODO: replace the description
                    };

                    for (auto i : options)
                    {
                        auto opt = std::make_shared< amc_option >(this,
                            _hw_monitor.get(),
                            i.second.first,
                            i.second.second);

                        depth_sensor.register_option(i.first, opt);
                        _advanced_options.push_back(i.first);
                    }

                    auto preset = std::make_shared< l535_preset_option >(
                        option_range{ RS2_L500_VISUAL_PRESET_CUSTOM,
                                      RS2_L500_VISUAL_PRESET_CUSTOM,
                                      1,
                                      RS2_L500_VISUAL_PRESET_CUSTOM },
                        "Preset to calibrate the camera to environment ambient, no ambient or low "
                        "ambient.");

                    depth_sensor.register_option(RS2_OPTION_VISUAL_PRESET, preset);
                });
            }
         } // namespace l535
    } // namespace ivcam2
} // namespace librealsense
