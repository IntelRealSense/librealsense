// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "ds-color-common.h"
#include "metadata.h"
#include <src/option.h>

#include <src/platform/uvc-option.h>
#include <src/ds/features/auto-exposure-roi-feature.h>
#include <src/metadata-parser.h>

#include <cstddef>

namespace librealsense
{
    using namespace ds;

    ds_color_common::ds_color_common( const std::shared_ptr< uvc_sensor > & raw_color_ep,
                                      synthetic_sensor & color_ep,
                                      firmware_version fw_version,
                                      std::shared_ptr< hw_monitor > hw_monitor,
                                      device * owner )
        : _raw_color_ep( raw_color_ep )
        , _color_ep( color_ep )
        , _fw_version( fw_version )
        , _hw_monitor( hw_monitor )
        , _owner( owner )
    {
    }

    void ds_color_common::register_color_options()
    {
        _color_ep.register_pu(RS2_OPTION_BRIGHTNESS);
        _color_ep.register_pu(RS2_OPTION_CONTRAST);
        _color_ep.register_pu(RS2_OPTION_SATURATION);
        _color_ep.register_pu(RS2_OPTION_GAMMA);
        _color_ep.register_pu(RS2_OPTION_SHARPNESS);

        auto white_balance_option = std::make_shared<uvc_pu_option>(_raw_color_ep, RS2_OPTION_WHITE_BALANCE);
        _color_ep.register_option(RS2_OPTION_WHITE_BALANCE, white_balance_option);
        auto auto_white_balance_option = std::make_shared<uvc_pu_option>(_raw_color_ep, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        _color_ep.register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, auto_white_balance_option);
        _color_ep.register_option(RS2_OPTION_WHITE_BALANCE,
            std::make_shared<auto_disabling_control>(
                white_balance_option,
                auto_white_balance_option));
    }

    void ds_color_common::register_standard_options()
    {
        auto gain_option = std::make_shared<uvc_pu_option>(_raw_color_ep, RS2_OPTION_GAIN);
        auto exposure_option = std::make_shared<uvc_pu_option>(_raw_color_ep, RS2_OPTION_EXPOSURE);
        auto auto_exposure_option = std::make_shared<uvc_pu_option>(_raw_color_ep, RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        _color_ep.register_option(RS2_OPTION_GAIN, gain_option);
        _color_ep.register_option(RS2_OPTION_EXPOSURE, exposure_option);
        _color_ep.register_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, auto_exposure_option);
        _color_ep.register_option(RS2_OPTION_EXPOSURE,
            std::make_shared<auto_disabling_control>(
                exposure_option,
                auto_exposure_option));
        _color_ep.register_option(RS2_OPTION_GAIN,
            std::make_shared<auto_disabling_control>(
                gain_option,
                auto_exposure_option));
    }

    void ds_color_common::register_metadata()
    {
        _color_ep.register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));
        _color_ep.register_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, std::make_shared<ds_md_attribute_actual_fps>());

        // attributes of md_capture_timing
        auto md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_timing);

        _color_ep.register_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, make_attribute_parser(&md_capture_timing::frame_counter, md_capture_timing_attributes::frame_counter_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP, make_rs400_sensor_ts_parser(make_uvc_header_parser(&platform::uvc_header::timestamp),
            make_attribute_parser(&md_capture_timing::sensor_timestamp, md_capture_timing_attributes::sensor_timestamp_attribute, md_prop_offset)));

        // attributes of md_rgb_control
        md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        _color_ep.register_metadata(RS2_FRAME_METADATA_GAIN_LEVEL, make_attribute_parser(&md_rgb_control::gain, md_rgb_control_attributes::gain_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE, make_attribute_parser(&md_rgb_control::manual_exp, md_rgb_control_attributes::manual_exp_attribute, md_prop_offset));

        // attributes of md_capture_stats
        md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_capture_stats);

        _color_ep.register_metadata(RS2_FRAME_METADATA_WHITE_BALANCE, make_attribute_parser(&md_capture_stats::white_balance, md_capture_stat_attributes::white_balance_attribute, md_prop_offset));

        // attributes of md_rgb_control
        md_prop_offset = metadata_raw_mode_offset +
            offsetof(md_rgb_mode, rgb_mode) +
            offsetof(md_rgb_normal_mode, intel_rgb_control);

        _color_ep.register_metadata(RS2_FRAME_METADATA_BRIGHTNESS,
            make_attribute_parser(&md_rgb_control::brightness, md_rgb_control_attributes::brightness_attribute, md_prop_offset,
                [](const rs2_metadata_type& param) {
                    // cast to short in order to return negative values
                    return *(short*)&(param);
                }));
        _color_ep.register_metadata(RS2_FRAME_METADATA_CONTRAST, make_attribute_parser(&md_rgb_control::contrast, md_rgb_control_attributes::contrast_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_SATURATION, make_attribute_parser(&md_rgb_control::saturation, md_rgb_control_attributes::saturation_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_SHARPNESS, make_attribute_parser(&md_rgb_control::sharpness, md_rgb_control_attributes::sharpness_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE, make_attribute_parser(&md_rgb_control::awb_temp, md_rgb_control_attributes::awb_temp_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::backlight_comp, md_rgb_control_attributes::backlight_comp_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_GAMMA, make_attribute_parser(&md_rgb_control::gamma, md_rgb_control_attributes::gamma_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_HUE, make_attribute_parser(&md_rgb_control::hue, md_rgb_control_attributes::hue_attribute, md_prop_offset,
            [](const rs2_metadata_type& param) {
                // cast to short in order to return negative values
                return *(short*)&(param);
            }));
        _color_ep.register_metadata(RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE, make_attribute_parser(&md_rgb_control::manual_wb, md_rgb_control_attributes::manual_wb_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_POWER_LINE_FREQUENCY, make_attribute_parser(&md_rgb_control::power_line_frequency, md_rgb_control_attributes::power_line_frequency_attribute, md_prop_offset));
        _color_ep.register_metadata(RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION, make_attribute_parser(&md_rgb_control::low_light_comp, md_rgb_control_attributes::low_light_comp_attribute, md_prop_offset));
    }
}
