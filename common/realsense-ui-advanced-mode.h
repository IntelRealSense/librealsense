// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs_advanced_mode.hpp>
#include "types.h"

template<class T, class S>
inline void slider_int(const char* id, T* val, S T::* field, bool& to_set)
{
    ImGui::Text("%s", id);
    int temp = val->*field;
    int min = (val + 1)->*field;
    int max = (val + 2)->*field;

    std::string slider_id = rs2::to_string() << "##" << id;

    if (ImGui::SliderInt(slider_id.c_str(), &temp, min, max))
    {
        val->*field = temp;
        to_set = true;
    }
}

template<class T, class S>
inline void checkbox(const char* id, T* val, S T::* f, bool& to_set)
{
    bool temp = (val->*f) > 0;

    if (ImGui::Checkbox(id, &temp))
    {
        val->*f = temp ? 1 : 0;
        to_set = true;
    }
}

template<class T, class S>
inline void slider_float(const char* id, T* val, S T::* field, bool& to_set)
{
    ImGui::Text("%s", id);
    float temp = float(val->*field);
    float min = float((val + 1)->*field);
    float max = float((val + 2)->*field);

    std::string slider_id = rs2::to_string() << "##" << id;

    if (ImGui::SliderFloat(slider_id.c_str(), &temp, min, max))
    {
        val->*field = static_cast<S>(temp);
        to_set = true;
    }
}

template<class T>
struct param_group
{
    using group_type = T;
    T vals[3];
    bool update = false;
};

struct advanced_mode_control
{
    param_group<STDepthControlGroup> depth_controls;
    param_group<STRsm> rsm;
    param_group<STRauSupportVectorControl> rsvc;
    param_group<STColorControl> color_control;
    param_group<STRauColorThresholdsControl> rctc;
    param_group<STSloColorThresholdsControl> sctc;
    param_group<STSloPenaltyControl> spc;
    param_group<STHdad> hdad;
    param_group<STColorCorrection> cc;
    param_group<STDepthTableControl> depth_table;
    param_group<STAEControl> ae;
    param_group<STCensusRadius> census;
};

inline void draw_advanced_mode_controls(rs400::advanced_mode& advanced, advanced_mode_control& amc, bool& get_curr_advanced_controls)
{
    if (get_curr_advanced_controls)
    {
        for (int k = 0; k < 3; ++k)
        {
            // Get Current Algo Control Values
            amc.depth_controls.vals[k] = advanced.get_depth_control(k);
            amc.rsm.vals[k] = advanced.get_rsm(k);
            amc.rsvc.vals[k] = advanced.get_rau_support_vector_control(k);
            amc.color_control.vals[k] = advanced.get_color_control(k);
            amc.rctc.vals[k] = advanced.get_rau_thresholds_control(k);
            amc.sctc.vals[k] = advanced.get_slo_color_thresholds_control(k);
            amc.spc.vals[k] = advanced.get_slo_penalty_control(k);
            amc.cc.vals[k] = advanced.get_color_correction(k);
            amc.depth_table.vals[k] = advanced.get_depth_table(k);
            amc.census.vals[k] = advanced.get_census(k);
        }
        amc.hdad.vals[0] = advanced.get_hdad();
        amc.hdad.vals[1] = amc.hdad.vals[0]; //setting min/max to the same value
        amc.hdad.vals[2] = amc.hdad.vals[0]; //setting min/max to the same value
        amc.ae.vals[0] = advanced.get_ae_control();
        amc.ae.vals[1] = amc.ae.vals[0]; //setting min/max to the same value
        amc.ae.vals[2] = amc.ae.vals[0]; //setting min/max to the same value
        get_curr_advanced_controls = false;
    }

    if (ImGui::TreeNode("Depth Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_int("DS Second Peak Threshold", amc.depth_controls.vals, &STDepthControlGroup::deepSeaSecondPeakThreshold, to_set);
        slider_int("DS Neighbor Threshold", amc.depth_controls.vals, &STDepthControlGroup::deepSeaNeighborThreshold, to_set);
        slider_int("DS Median Threshold", amc.depth_controls.vals, &STDepthControlGroup::deepSeaMedianThreshold, to_set);
        slider_int("Estimate Median Increment", amc.depth_controls.vals, &STDepthControlGroup::plusIncrement, to_set);
        slider_int("Estimate Median Decrement", amc.depth_controls.vals, &STDepthControlGroup::minusDecrement, to_set);
        slider_int("Score Minimum Threshold", amc.depth_controls.vals, &STDepthControlGroup::scoreThreshA, to_set);
        slider_int("Score Maximum Threshold", amc.depth_controls.vals, &STDepthControlGroup::scoreThreshB, to_set);
        slider_int("DS LR Threshold", amc.depth_controls.vals, &STDepthControlGroup::lrAgreeThreshold, to_set);
        slider_int("Texture Count Threshold", amc.depth_controls.vals, &STDepthControlGroup::textureCountThreshold, to_set);
        slider_int("Texture Difference Threshold", amc.depth_controls.vals, &STDepthControlGroup::textureDifferenceThreshold, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_depth_control(amc.depth_controls.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Rsm"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        checkbox("RSM Bypass", amc.rsm.vals, &STRsm::rsmBypass, to_set);
        slider_float("Disparity Difference Threshold", amc.rsm.vals, &STRsm::diffThresh, to_set);
        slider_float("SLO RAU Difference Threshold", amc.rsm.vals, &STRsm::sloRauDiffThresh, to_set);
        slider_int("Remove Threshold", amc.rsm.vals, &STRsm::removeThresh, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_rsm(amc.rsm.vals[0]);

        ImGui::TreePop();
    }


    if (ImGui::TreeNode("Rau Support Vector Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_int("Min West", amc.rsvc.vals, &STRauSupportVectorControl::minWest, to_set);
        slider_int("Min East", amc.rsvc.vals, &STRauSupportVectorControl::minEast, to_set);
        slider_int("Min WE Sum", amc.rsvc.vals, &STRauSupportVectorControl::minWEsum, to_set);
        slider_int("Min North", amc.rsvc.vals, &STRauSupportVectorControl::minNorth, to_set);
        slider_int("Min South", amc.rsvc.vals, &STRauSupportVectorControl::minSouth, to_set);
        slider_int("Min NS Sum", amc.rsvc.vals, &STRauSupportVectorControl::minNSsum, to_set);
        slider_int("U Shrink", amc.rsvc.vals, &STRauSupportVectorControl::uShrink, to_set);
        slider_int("V Shrink", amc.rsvc.vals, &STRauSupportVectorControl::vShrink, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_rau_support_vector_control(amc.rsvc.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Color Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        checkbox("Disable SAD Color", amc.color_control.vals, &STColorControl::disableSADColor, to_set);
        checkbox("Disable RAU Color", amc.color_control.vals, &STColorControl::disableRAUColor, to_set);
        checkbox("Disable SLO Right Color", amc.color_control.vals, &STColorControl::disableSLORightColor, to_set);
        checkbox("Disable SLO Left Color", amc.color_control.vals, &STColorControl::disableSLOLeftColor, to_set);
        checkbox("Disable SAD Normalize", amc.color_control.vals, &STColorControl::disableSADNormalize, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_color_control(amc.color_control.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Rau Color Thresholds Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_int("Diff Threshold Red", amc.rctc.vals, &STRauColorThresholdsControl::rauDiffThresholdRed, to_set);
        slider_int("Diff Threshold Green", amc.rctc.vals, &STRauColorThresholdsControl::rauDiffThresholdGreen, to_set);
        slider_int("Diff Threshold Blue", amc.rctc.vals, &STRauColorThresholdsControl::rauDiffThresholdBlue, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_rau_thresholds_control(amc.rctc.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("SLO Color Thresholds Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_int("Diff Threshold Red", amc.sctc.vals, &STSloColorThresholdsControl::diffThresholdRed, to_set);
        slider_int("Diff Threshold Green", amc.sctc.vals, &STSloColorThresholdsControl::diffThresholdGreen, to_set);
        slider_int("Diff Threshold Blue", amc.sctc.vals, &STSloColorThresholdsControl::diffThresholdBlue, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_slo_color_thresholds_control(amc.sctc.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("SLO Penalty Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_int("K1 Penalty", amc.spc.vals, &STSloPenaltyControl::sloK1Penalty, to_set);
        slider_int("K2 Penalty", amc.spc.vals, &STSloPenaltyControl::sloK2Penalty, to_set);
        slider_int("K1 Penalty Mod1", amc.spc.vals, &STSloPenaltyControl::sloK1PenaltyMod1, to_set);
        slider_int("K1 Penalty Mod2", amc.spc.vals, &STSloPenaltyControl::sloK1PenaltyMod2, to_set);
        slider_int("K2 Penalty Mod1", amc.spc.vals, &STSloPenaltyControl::sloK2PenaltyMod1, to_set);
        slider_int("K2 Penalty Mod2", amc.spc.vals, &STSloPenaltyControl::sloK2PenaltyMod2, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_slo_penalty_control(amc.spc.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("HDAD"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        checkbox("Ignore SAD", amc.hdad.vals, &STHdad::ignoreSAD, to_set);

        // TODO: Not clear from documents what is the valid range:
        slider_float("AD Lambda", amc.hdad.vals, &STHdad::lambdaAD, to_set);
        slider_float("Census Lambda", amc.hdad.vals, &STHdad::lambdaCensus, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_hdad(amc.hdad.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Color Correction"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_float("Color Correction 1", amc.cc.vals, &STColorCorrection::colorCorrection1,  to_set);
        slider_float("Color Correction 2", amc.cc.vals, &STColorCorrection::colorCorrection2,  to_set);
        slider_float("Color Correction 3", amc.cc.vals, &STColorCorrection::colorCorrection3,  to_set);
        slider_float("Color Correction 4", amc.cc.vals, &STColorCorrection::colorCorrection4,  to_set);
        slider_float("Color Correction 5", amc.cc.vals, &STColorCorrection::colorCorrection5,  to_set);
        slider_float("Color Correction 6", amc.cc.vals, &STColorCorrection::colorCorrection6,  to_set);
        slider_float("Color Correction 7", amc.cc.vals, &STColorCorrection::colorCorrection7,  to_set);
        slider_float("Color Correction 8", amc.cc.vals, &STColorCorrection::colorCorrection8,  to_set);
        slider_float("Color Correction 9", amc.cc.vals, &STColorCorrection::colorCorrection9,  to_set);
        slider_float("Color Correction 10",amc.cc.vals, &STColorCorrection::colorCorrection10, to_set);
        slider_float("Color Correction 11",amc.cc.vals, &STColorCorrection::colorCorrection11, to_set);
        slider_float("Color Correction 12",amc.cc.vals, &STColorCorrection::colorCorrection12, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_color_correction(amc.cc.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Depth Table"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_float("Depth Units", amc.depth_table.vals, &STDepthTableControl::depthUnits, to_set);
        slider_float("Depth Clamp Min", amc.depth_table.vals, &STDepthTableControl::depthClampMin, to_set);
        slider_float("Depth Clamp Max", amc.depth_table.vals, &STDepthTableControl::depthClampMax, to_set);
        slider_float("Disparity Mode", amc.depth_table.vals, &STDepthTableControl::disparityMode, to_set);
        slider_float("Disparity Shift", amc.depth_table.vals, &STDepthTableControl::disparityShift, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_depth_table(amc.depth_table.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("AE Control"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_float("Mean Intensity Set Point", amc.ae.vals, &STAEControl::meanIntensitySetPoint, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_ae_control(amc.ae.vals[0]);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Census Enable Reg"))
    {
        ImGui::PushItemWidth(-1);

        auto to_set = false;

        slider_float("u-Diameter", amc.census.vals, &STCensusRadius::uDiameter, to_set);
        slider_float("v-Diameter", amc.census.vals, &STCensusRadius::vDiameter, to_set);

        ImGui::PopItemWidth();

        if (to_set)
            advanced.set_census(amc.census.vals[0]);

        ImGui::TreePop();
    }
}

