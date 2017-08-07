// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <fstream>

#include <chrono>
#include <thread>
#include <sstream>
#include <map>
#include <string>
#include <iomanip>

#include "json.hpp"
#include <librealsense/rs2_advanced_mode_command.h>
#include "types.h"
#include "presets.h"

namespace librealsense
{
    using json = nlohmann::json;

    template<class T>
    struct param_group
    {
        using group_type = T;
        T vals[3];
        bool update = false;
    };

    struct preset_param_group{
        param_group<STDepthControlGroup>         depth_controls;
        param_group<STRsm>                       rsm;
        param_group<STRauSupportVectorControl>   rsvc;
        param_group<STColorControl>              color_control;
        param_group<STRauColorThresholdsControl> rctc;
        param_group<STSloColorThresholdsControl> sctc;
        param_group<STSloPenaltyControl>         spc;
        param_group<STHdad>                      hdad;
        param_group<STColorCorrection>           cc;
        param_group<STDepthTableControl>         depth_table;
        param_group<STAEControl>                 ae;
        param_group<STCensusRadius>              census;

        preset_param_group(const preset& other)
        {
            copy(other);
        }

        preset_param_group& operator=(const preset& other)
        {
            copy(other);
            return *this;
        }
    private:
        void copy(const preset& other)
        {
            depth_controls.vals[0] = other.depth_controls;
            rsm.vals[0] = other.rsm;
            rsvc.vals[0] = other.rsvc;
            color_control.vals[0] = other.color_control;
            rctc.vals[0] = other.rctc;
            sctc.vals[0] = other.sctc;
            spc.vals[0] = other.spc;
            hdad.vals[0] = other.hdad;
            cc.vals[0] = other.cc;
            depth_table.vals[0] = other.depth_table;
            ae.vals[0] = other.ae;
            census.vals[0] = other.census;
        }
    };

    struct json_feild
    {
        virtual ~json_feild() = default;

        bool was_set = false;

        virtual void load(const std::string& value) = 0;
        virtual std::string save() const = 0;
    };

    template<class T, class S>
    struct json_struct_feild : json_feild
    {
        T* strct;
        S T::group_type::* field;
        float scale = 1.0f;
        bool check_ranges = true;

        void load(const std::string& str) override
        {
            float value = ::atof(str.c_str());
            strct->vals[0].*field = (scale * value);
            strct->update = true;
        }

        std::string save() const override
        {
            std::stringstream ss;
            ss << strct->vals[0].*field / scale;
            return ss.str();
        }
    };


    template<class T, class S>
    struct json_invert_struct_feild : json_struct_feild<T, S>
    {
        using json_struct_feild<T, S>::strct;
        using json_struct_feild<T, S>::field;

        void load(const std::string& str) override
        {
            float value = ::atof(str.c_str());
            (strct->vals[0].*field) = (value > 0) ? 0 : 1;
            strct->update = true;
        }

        std::string save() const override
        {
            return strct->vals[0].*field > 0 ? "0" : "1";
        }
    };

    template<class T, class S>
    std::shared_ptr<json_feild> make_feild(T& strct, S T::group_type::* feild, double scale = 1.0f)
    {
        std::shared_ptr<json_struct_feild<T, S>> f(new json_struct_feild<T, S>());
        f->field = feild;
        f->strct = &strct;
        f->scale = scale;
        return f;
    }

    template<class T, class S>
    std::shared_ptr<json_feild> make_invert_feild(T& strct, S T::group_type::* feild)
    {
        std::shared_ptr<json_invert_struct_feild<T, S>> f(new json_invert_struct_feild<T, S>());
        f->field = feild;
        f->strct = &strct;
        return f;
    }

    typedef std::map<std::string, std::shared_ptr<json_feild>> parsers_map;

    inline parsers_map initialize_feild_parsers(preset_param_group& p)
    {
        return {
            // Depth Control
            { "lrAgreeThreshold",           make_feild(p.depth_controls, &STDepthControlGroup::lrAgreeThreshold) },
            { "scoreThreshB",               make_feild(p.depth_controls, &STDepthControlGroup::scoreThreshB) },
            { "deepSeaMedianThreshold",     make_feild(p.depth_controls, &STDepthControlGroup::deepSeaMedianThreshold) },
            { "scoreThreshA",               make_feild(p.depth_controls, &STDepthControlGroup::scoreThreshA) },
            { "deepSeaNeighborThreshold",   make_feild(p.depth_controls, &STDepthControlGroup::deepSeaNeighborThreshold) },
            { "deepSeaSecondPeakThreshold", make_feild(p.depth_controls, &STDepthControlGroup::deepSeaSecondPeakThreshold) },
            { "textureDifferenceThreshold", make_feild(p.depth_controls, &STDepthControlGroup::textureDifferenceThreshold) },
            { "textureCountThreshold",      make_feild(p.depth_controls, &STDepthControlGroup::textureCountThreshold) },
            { "minusDecrement",             make_feild(p.depth_controls, &STDepthControlGroup::minusDecrement) },
            { "plusIncrement",              make_feild(p.depth_controls, &STDepthControlGroup::plusIncrement) },

            // RSM
            { "rsmBypass",        make_invert_feild(p.rsm, &STRsm::rsmBypass) },
            { "diffThresh",       make_feild(p.rsm, &STRsm::diffThresh) },
            { "sloRauDiffThresh", make_feild(p.rsm, &STRsm::sloRauDiffThresh) },
            { "removeThresh",     make_feild(p.rsm, &STRsm::removeThresh) },

            // RAU Support Vector Control
            { "minEast",  make_feild(p.rsvc, &STRauSupportVectorControl::minEast) },
            { "minNorth", make_feild(p.rsvc, &STRauSupportVectorControl::minNorth) },
            { "minNSsum", make_feild(p.rsvc, &STRauSupportVectorControl::minNSsum) },
            { "minSouth", make_feild(p.rsvc, &STRauSupportVectorControl::minSouth) },
            { "minWest",  make_feild(p.rsvc, &STRauSupportVectorControl::minWest) },
            { "minWEsum", make_feild(p.rsvc, &STRauSupportVectorControl::minWEsum) },
            { "uShrink",  make_feild(p.rsvc, &STRauSupportVectorControl::uShrink) },
            { "vShrink",  make_feild(p.rsvc, &STRauSupportVectorControl::vShrink) },

            // Color Controls
            { "disableRAUColor",      make_feild(p.color_control, &STColorControl::disableRAUColor) },
            { "disableSADColor",      make_feild(p.color_control, &STColorControl::disableSADColor) },
            { "disableSADNormalize",  make_feild(p.color_control, &STColorControl::disableSADNormalize) },
            { "disableSLOLeftColor",  make_feild(p.color_control, &STColorControl::disableSLOLeftColor) },
            { "disableSLORightColor", make_feild(p.color_control, &STColorControl::disableSLORightColor) },

            // RAU Color Thresholds Control
            { "rauDiffThresholdBlue",  make_feild(p.rctc, &STRauColorThresholdsControl::rauDiffThresholdBlue) },
            { "rauDiffThresholdGreen", make_feild(p.rctc, &STRauColorThresholdsControl::rauDiffThresholdGreen) },
            { "rauDiffThresholdRed",   make_feild(p.rctc, &STRauColorThresholdsControl::rauDiffThresholdRed) },

            // SLO Color Thresholds Control
            { "diffThresholdBlue",  make_feild(p.sctc, &STSloColorThresholdsControl::diffThresholdBlue) },
            { "diffThresholdGreen", make_feild(p.sctc, &STSloColorThresholdsControl::diffThresholdGreen) },
            { "diffThresholdRed",   make_feild(p.sctc, &STSloColorThresholdsControl::diffThresholdRed) },

            // SLO Penalty Control
            { "sloK1Penalty",     make_feild(p.spc, &STSloPenaltyControl::sloK1Penalty) },
            { "sloK1PenaltyMod1", make_feild(p.spc, &STSloPenaltyControl::sloK1PenaltyMod1) },
            { "sloK1PenaltyMod2", make_feild(p.spc, &STSloPenaltyControl::sloK1PenaltyMod2) },
            { "sloK2Penalty",     make_feild(p.spc, &STSloPenaltyControl::sloK2Penalty) },
            { "sloK2PenaltyMod1", make_feild(p.spc, &STSloPenaltyControl::sloK2PenaltyMod1) },
            { "sloK2PenaltyMod2", make_feild(p.spc, &STSloPenaltyControl::sloK2PenaltyMod2) },

            // HDAD
            { "lambdaAD",     make_feild(p.hdad, &STHdad::lambdaAD) },
            { "lambdaCensus", make_feild(p.hdad, &STHdad::lambdaCensus) },
            { "ignoreSAD",    make_feild(p.hdad, &STHdad::ignoreSAD) },

            // SLO Penalty Control
            { "colorCorrection1",  make_feild(p.cc, &STColorCorrection::colorCorrection1) },
            { "colorCorrection2",  make_feild(p.cc, &STColorCorrection::colorCorrection2) },
            { "colorCorrection3",  make_feild(p.cc, &STColorCorrection::colorCorrection3) },
            { "colorCorrection4",  make_feild(p.cc, &STColorCorrection::colorCorrection4) },
            { "colorCorrection5",  make_feild(p.cc, &STColorCorrection::colorCorrection5) },
            { "colorCorrection6",  make_feild(p.cc, &STColorCorrection::colorCorrection6) },
            { "colorCorrection7",  make_feild(p.cc, &STColorCorrection::colorCorrection7) },
            { "colorCorrection8",  make_feild(p.cc, &STColorCorrection::colorCorrection8) },
            { "colorCorrection9",  make_feild(p.cc, &STColorCorrection::colorCorrection9) },
            { "colorCorrection10", make_feild(p.cc, &STColorCorrection::colorCorrection10) },
            { "colorCorrection11", make_feild(p.cc, &STColorCorrection::colorCorrection11) },
            { "colorCorrection12", make_feild(p.cc, &STColorCorrection::colorCorrection12) },

            // Depth Table
            { "depthUnits",     make_feild(p.depth_table, &STDepthTableControl::depthUnits) },
            { "depthClampMin",  make_feild(p.depth_table, &STDepthTableControl::depthClampMin) },
            { "depthClampMax",  make_feild(p.depth_table, &STDepthTableControl::depthClampMax) },
            { "disparityMode",  make_feild(p.depth_table, &STDepthTableControl::disparityMode) },
            { "disparityShift", make_feild(p.depth_table, &STDepthTableControl::disparityShift) },

            // Auto-Exposure
            { "meanIntensitySetPoint", make_feild(p.ae, &STAEControl::meanIntensitySetPoint) },

            // Census
            { "uDiameter", make_feild(p.census, &STCensusRadius::uDiameter) },
            { "vDiameter", make_feild(p.census, &STCensusRadius::vDiameter) } };
    }


    inline std::vector<uint8_t> generate_json(const preset& in_preset)
    {
        preset_param_group p = in_preset;
        auto fields = initialize_feild_parsers(p);

        json j;
        for (auto&& f : fields)
        {
            j[f.first.c_str()] = f.second->save();
        }

        auto str = j.dump();
        return std::vector<uint8_t>(str.begin(), str.end());
    }



    inline void update_structs(const std::string& content, preset& in_preset)
    {
        preset_param_group p = in_preset;
        json j = json::parse(content);
        auto fields = initialize_feild_parsers(p);

        for (auto it = j.begin(); it != j.end(); ++it)
        {
            auto kvp = fields.find(it.key());
            if (kvp != fields.end())
            {
                try
                {
                    if (it.value().type() != nlohmann::basic_json<>::value_t::string)
                    {
                        float val = it.value();
                        std::stringstream ss;
                        ss << val;
                        kvp->second->load(ss.str());
                    }
                    else
                    {
                        kvp->second->load(it.value());
                    }
                    kvp->second->was_set = true;
                }
                catch (...)
                {
                    throw invalid_value_exception(to_string() << "Couldn't set \"" << it.key());
                }
            }
        }

        in_preset.depth_controls = p.depth_controls.vals[0];
        in_preset.rsm = p.rsm.vals[0];
        in_preset.rsvc = p.rsvc.vals[0];
        in_preset.color_control = p.color_control.vals[0];
        in_preset.rctc = p.rctc.vals[0];
        in_preset.sctc = p.sctc.vals[0];
        in_preset.spc = p.spc.vals[0];
        in_preset.hdad = p.hdad.vals[0];
        in_preset.cc = p.cc.vals[0];
        in_preset.depth_table = p.depth_table.vals[0];
        in_preset.ae = p.ae.vals[0];
        in_preset.census = p.census.vals[0];
    }
}
