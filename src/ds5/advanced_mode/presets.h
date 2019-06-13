// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../../../include/librealsense2/h/rs_advanced_mode_command.h"

namespace librealsense
{
    typedef struct
    {
        float laser_power;
        bool was_set = false;
    }laser_power_control;

    typedef struct
    {
        int laser_state;
        bool was_set = false;
    }laser_state_control;

    typedef struct
    {
        float exposure;
        bool was_set = false;
    }exposure_control;

    typedef struct
    {
        int auto_exposure;
        bool was_set = false;
    }auto_exposure_control;

    typedef struct
    {
        float gain;
        bool was_set = false;
    }gain_control;

    typedef struct
    {
        int backlight_compensation;
        bool was_set = false;
    }backlight_compensation_control;

    typedef struct
    {
        float brightness;
        bool was_set = false;
    }brightness_control;

    typedef struct
    {
        float contrast;
        bool was_set = false;
    }contrast_control;

    typedef struct
    {
        float gamma;
        bool was_set = false;
    }gamma_control;

    typedef struct
    {
        float hue;
        bool was_set = false;
    }hue_control;

    typedef struct
    {
        float saturation;
        bool was_set = false;
    }saturation_control;

    typedef struct
    {
        float sharpness;
        bool was_set = false;
    }sharpness_control;

    typedef struct
    {
        float white_balance;
        bool was_set = false;
    }white_balance_control;

    typedef struct
    {
        int auto_white_balance;
        bool was_set = false;
    }auto_white_balance_control;

    typedef struct
    {
        int power_line_frequency;
        bool was_set = false;
    }power_line_frequency_control;

    struct preset{
        STDepthControlGroup            depth_controls;
        STRsm                          rsm;
        STRauSupportVectorControl      rsvc;
        STColorControl                 color_control;
        STRauColorThresholdsControl    rctc;
        STSloColorThresholdsControl    sctc;
        STSloPenaltyControl            spc;
        STHdad                         hdad;
        STColorCorrection              cc;
        STDepthTableControl            depth_table;
        STAEControl                    ae;
        STCensusRadius                 census;
        STAFactor                      amplitude_factor;
        laser_state_control            laser_state;
        laser_power_control            laser_power;
        exposure_control               depth_exposure;
        auto_exposure_control          depth_auto_exposure;
        gain_control                   depth_gain;
        auto_white_balance_control     depth_auto_white_balance;
        exposure_control               color_exposure;
        auto_exposure_control          color_auto_exposure;
        backlight_compensation_control color_backlight_compensation;
        brightness_control             color_brightness;
        contrast_control               color_contrast;
        gain_control                   color_gain;
        gamma_control                  color_gamma;
        hue_control                    color_hue;
        saturation_control             color_saturation;
        sharpness_control              color_sharpness;
        white_balance_control          color_white_balance;
        auto_white_balance_control     color_auto_white_balance;
        power_line_frequency_control   color_power_line_frequency;
    };

    void default_400(preset& p);
    void default_405(preset& p);
    void default_410(preset& p);
    void default_420(preset& p);
    void default_430(preset& p);
    void high_res_high_accuracy(preset& p);
    void high_res_high_density(preset& p);
    void high_res_mid_density(preset& p);
    void low_res_high_accuracy(preset& p);
    void low_res_high_density(preset& p);
    void low_res_mid_density(preset& p);
    void mid_res_high_accuracy(preset& p);
    void mid_res_high_density(preset& p);
    void mid_res_mid_density(preset& p);
    void hand_gesture(preset& p);
    void d415_remove_ir(preset& p);
    void d460_remove_ir(preset& p);
}
