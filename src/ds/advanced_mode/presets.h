// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../../../include/librealsense2/h/rs_advanced_mode_command.h"

namespace librealsense
{
    typedef struct laser_power_control
    {
        float laser_power;
        bool was_set = false;
    }laser_power_control;

    typedef struct laser_state_control
    {
        int laser_state;
        bool was_set = false;
    }laser_state_control;

    typedef struct exposure_control
    {
        float exposure;
        bool was_set = false;
    }exposure_control;

    typedef struct auto_exposure_control
    {
        int auto_exposure;
        bool was_set = false;
    }auto_exposure_control;

    typedef struct gain_control
    {
        float gain;
        bool was_set = false;
    }gain_control;

    typedef struct backlight_compensation_control
    {
        int backlight_compensation;
        bool was_set = false;
    }backlight_compensation_control;

    typedef struct brightness_control
    {
        float brightness;
        bool was_set = false;
    }brightness_control;

    typedef struct contrast_control
    {
        float contrast;
        bool was_set = false;
    }contrast_control;

    typedef struct gamma_control
    {
        float gamma;
        bool was_set = false;
    }gamma_control;

    typedef struct hue_control
    {
        float hue;
        bool was_set = false;
    }hue_control;

    typedef struct saturation_control
    {
        float saturation;
        bool was_set = false;
    }saturation_control;

    typedef struct sharpness_control
    {
        float sharpness;
        bool was_set = false;
    }sharpness_control;

    typedef struct white_balance_control
    {
        float white_balance;
        bool was_set = false;
    }white_balance_control;

    typedef struct auto_white_balance_control
    {
        int auto_white_balance;
        bool was_set = false;
    }auto_white_balance_control;

    typedef struct power_line_frequency_control
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
    void default_405u(preset& p);
    void default_405(preset& p);
    void default_410(preset& p);
    void default_420(preset& p);
    void default_430(preset& p);
    void default_450_high_res(preset& p);
    void default_450_mid_low_res(preset& p);
    void high_accuracy(preset& p);
    void high_density(preset& p);
    void mid_density(preset& p);
    void hand_gesture(preset& p);
    void d415_remove_ir(preset& p);
    void d460_remove_ir(preset& p);
}
