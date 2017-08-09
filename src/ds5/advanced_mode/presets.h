// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "../../../include/librealsense2/h/rs_advanced_mode_command.h"

namespace librealsense
{
    struct preset{
        STDepthControlGroup         depth_controls;
        STRsm                       rsm;
        STRauSupportVectorControl   rsvc;
        STColorControl              color_control;
        STRauColorThresholdsControl rctc;
        STSloColorThresholdsControl sctc;
        STSloPenaltyControl         spc;
        STHdad                      hdad;
        STColorCorrection           cc;
        STDepthTableControl         depth_table;
        STAEControl                 ae;
        STCensusRadius              census;
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
