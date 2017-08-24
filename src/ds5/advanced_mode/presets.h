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
        laser_state_control            laser_state;
        laser_power_control            laser_power;
        exposure_control               depth_exposure;
        auto_exposure_control          depth_auto_exposure;
        gain_control                   depth_gain;
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

    void generic_ds5_passive_full_seed_config(preset& p);
    void generic_ds5_passive_vga_seed_config(preset& p);
    void generic_ds5_passive_small_seed_config(preset& p);
    void generic_ds5_active_full_dense_depth(preset& p);
    void generic_ds5_active_full_generic_depth(preset& p);
    void generic_ds5_active_full_super_dense_depth(preset& p);
    void generic_ds5_active_full_accurate_depth(preset& p);
    void generic_ds5_active_vga_generic_depth(preset& p);
    void generic_ds5_active_vga_dense_depth(preset& p);
    void generic_ds5_active_vga_accurate_depth(preset& p);
    void generic_ds5_active_small_dense_depth(preset& p);
    void generic_ds5_awg_full_dense_depth(preset& p);
    void generic_ds5_awg_vga_dense_depth(preset& p);
    void generic_ds5_awg_small_dense_depth(preset& p);
    void specific_ds5_passive_full_face(preset& p);
    void specific_ds5_passive_full_box(preset& p);
    void specific_ds5_passive_full_hand_gesture(preset& p);
    void specific_ds5_passive_full_indoor_passive(preset& p);
    void specific_ds5_passive_full_outdoor_passive(preset& p);
    void specific_ds5_passive_vga_face(preset& p);
    void specific_ds5_passive_vga_box(preset& p);
    void specific_ds5_passive_vga_hand_gesture(preset& p);
    void specific_ds5_passive_vga_indoor_passive(preset& p);
    void specific_ds5_passive_vga_outdoor_passive(preset& p);
    void specific_ds5_passive_small_face(preset& p);
    void specific_ds5_passive_small_box(preset& p);
    void specific_ds5_passive_small_hand_gesture(preset& p);
    void specific_ds5_passive_small_indoor_passive(preset& p);
    void specific_ds5_passive_small_outdoor_passive(preset& p);
    void specific_ds5_active_full_outdoor(preset& p);
    void specific_ds5_active_full_face(preset& p);
    void specific_ds5_active_full_box(preset& p);
    void specific_ds5_active_full_hand_gesture(preset& p);
    void specific_ds5_active_full_3d_body_scan(preset& p);
    void specific_ds5_active_full_indoor(preset& p);
    void specific_ds5_active_vga_outdoor(preset& p);
    void specific_ds5_active_vga_face(preset& p);
    void specific_ds5_active_vga_box(preset& p);
    void specific_ds5_active_vga_hand_gesture(preset& p);
    void specific_ds5_active_vga_indoor(preset& p);
    void specific_ds5_active_small_outdoor(preset& p);
    void specific_ds5_active_small_floor_low(preset& p);
    void specific_ds5_active_small_face(preset& p);
    void specific_ds5_active_small_box(preset& p);
    void specific_ds5_active_small_hand_gesture(preset& p);
    void specific_ds5_active_small_indoor(preset& p);
    void specific_ds5_awg_full_outdoor(preset& p);
    void specific_ds5_awg_full_face(preset& p);
    void specific_ds5_awg_full_box(preset& p);
    void specific_ds5_awg_full_hand_gesture(preset& p);
    void specific_ds5_awg_full_indoor(preset& p);
    void specific_ds5_awg_vga_outdoor(preset& p);
    void specific_ds5_awg_vga_face(preset& p);
    void specific_ds5_awg_vga_box(preset& p);
    void specific_ds5_awg_vga_hand_gesture(preset& p);
    void specific_ds5_awg_vga_indoor(preset& p);
    void specific_ds5_awg_small_outdoor(preset& p);
    void specific_ds5_awg_small_face(preset& p);
    void specific_ds5_awg_small_box(preset& p);
    void specific_ds5_awg_small_hand_gesture(preset& p);
    void specific_ds5_awg_small_indoor(preset& p);
}
