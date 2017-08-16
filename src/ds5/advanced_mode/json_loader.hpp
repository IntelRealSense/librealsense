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

#include "../../../third-party/json.hpp"
#include <librealsense/h/rs2_advanced_mode_command.h>
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
            { "param-leftrightthreshold", make_feild(p.depth_controls, &STDepthControlGroup::lrAgreeThreshold) },
            { "param-maxscorethreshb", make_feild(p.depth_controls, &STDepthControlGroup::scoreThreshB) },
            { "param-medianthreshold", make_feild(p.depth_controls, &STDepthControlGroup::deepSeaMedianThreshold) },
            { "param-minscorethresha", make_feild(p.depth_controls, &STDepthControlGroup::scoreThreshA) },
            { "param-neighborthresh", make_feild(p.depth_controls, &STDepthControlGroup::deepSeaNeighborThreshold) },
            { "param-secondpeakdelta", make_feild(p.depth_controls, &STDepthControlGroup::deepSeaSecondPeakThreshold) },
            { "param-texturecountthresh", make_feild(p.depth_controls, &STDepthControlGroup::textureCountThreshold) },
            { "param-robbinsmonrodecrement", make_feild(p.depth_controls, &STDepthControlGroup::minusDecrement) },
            { "param-robbinsmonroincrement", make_feild(p.depth_controls, &STDepthControlGroup::plusIncrement) },

            // RSM
            { "param-usersm", make_invert_feild(p.rsm, &STRsm::rsmBypass) },
            { "param-rsmdiffthreshold", make_feild(p.rsm, &STRsm::diffThresh) },
            { "param-rsmrauslodiffthreshold", make_feild(p.rsm, &STRsm::sloRauDiffThresh) },
            { "param-rsmremovethreshold", make_feild(p.rsm, &STRsm::removeThresh) },

            // RAU Support Vector Control
            { "param-raumine", make_feild(p.rsvc, &STRauSupportVectorControl::minEast) },
            { "param-rauminn", make_feild(p.rsvc, &STRauSupportVectorControl::minNorth) },
            { "param-rauminnssum", make_feild(p.rsvc, &STRauSupportVectorControl::minNSsum) },
            { "param-raumins", make_feild(p.rsvc, &STRauSupportVectorControl::minSouth) },
            { "param-rauminw", make_feild(p.rsvc, &STRauSupportVectorControl::minWest) },
            { "param-rauminwesum", make_feild(p.rsvc, &STRauSupportVectorControl::minWEsum) },
            { "param-regionshrinku", make_feild(p.rsvc, &STRauSupportVectorControl::uShrink) },
            { "param-regionshrinkv", make_feild(p.rsvc, &STRauSupportVectorControl::vShrink) },

            // Color Controls
            { "param-disableraucolor", make_feild(p.color_control, &STColorControl::disableRAUColor) },
            { "param-disablesadcolor", make_feild(p.color_control, &STColorControl::disableSADColor) },
            { "param-disablesadnormalize", make_feild(p.color_control, &STColorControl::disableSADNormalize) },
            { "param-disablesloleftcolor", make_feild(p.color_control, &STColorControl::disableSLOLeftColor) },
            { "param-disableslorightcolor", make_feild(p.color_control, &STColorControl::disableSLORightColor) },

            // RAU Color Thresholds Control
            { "param-regioncolorthresholdb", make_feild(p.rctc, &STRauColorThresholdsControl::rauDiffThresholdBlue) },
            { "param-regioncolorthresholdg", make_feild(p.rctc, &STRauColorThresholdsControl::rauDiffThresholdGreen) },
            { "param-regioncolorthresholdr", make_feild(p.rctc, &STRauColorThresholdsControl::rauDiffThresholdRed) },

            // SLO Color Thresholds Control
            { "param-scanlineedgetaub", make_feild(p.sctc, &STSloColorThresholdsControl::diffThresholdBlue) },
            { "param-scanlineedgetaug", make_feild(p.sctc, &STSloColorThresholdsControl::diffThresholdGreen) },
            { "param-scanlineedgetaur", make_feild(p.sctc, &STSloColorThresholdsControl::diffThresholdRed) },

            // SLO Penalty Control
            { "param-scanlinep1", make_feild(p.spc, &STSloPenaltyControl::sloK1Penalty) },
            { "param-scanlinep1onediscon", make_feild(p.spc, &STSloPenaltyControl::sloK1PenaltyMod1) },
            { "param-scanlinep1twodiscon", make_feild(p.spc, &STSloPenaltyControl::sloK1PenaltyMod2) },
            { "param-scanlinep2", make_feild(p.spc, &STSloPenaltyControl::sloK2Penalty) },
            { "param-scanlinep2onediscon", make_feild(p.spc, &STSloPenaltyControl::sloK2PenaltyMod1) },
            { "param-scanlinep2twodiscon", make_feild(p.spc, &STSloPenaltyControl::sloK2PenaltyMod2) },

            // HDAD
            { "param-lambdaad", make_feild(p.hdad, &STHdad::lambdaAD) },
            { "param-lambdacensus", make_feild(p.hdad, &STHdad::lambdaCensus) },
            { "ignoreSAD", make_feild(p.hdad, &STHdad::ignoreSAD) },

            // SLO Penalty Control
            { "param-colorcorrection1", make_feild(p.cc, &STColorCorrection::colorCorrection1) },
            { "param-colorcorrection2", make_feild(p.cc, &STColorCorrection::colorCorrection2) },
            { "param-colorcorrection3", make_feild(p.cc, &STColorCorrection::colorCorrection3) },
            { "param-colorcorrection4", make_feild(p.cc, &STColorCorrection::colorCorrection4) },
            { "param-colorcorrection5", make_feild(p.cc, &STColorCorrection::colorCorrection5) },
            { "param-colorcorrection6", make_feild(p.cc, &STColorCorrection::colorCorrection6) },
            { "param-colorcorrection7", make_feild(p.cc, &STColorCorrection::colorCorrection7) },
            { "param-colorcorrection8", make_feild(p.cc, &STColorCorrection::colorCorrection8) },
            { "param-colorcorrection9", make_feild(p.cc, &STColorCorrection::colorCorrection9) },
            { "param-colorcorrection10", make_feild(p.cc, &STColorCorrection::colorCorrection10) },
            { "param-colorcorrection11", make_feild(p.cc, &STColorCorrection::colorCorrection11) },
            { "param-colorcorrection12", make_feild(p.cc, &STColorCorrection::colorCorrection12) },

            // Depth Table
            { "param-depthunits", make_feild(p.depth_table, &STDepthTableControl::depthUnits) },
            { "param-depthclampmin", make_feild(p.depth_table, &STDepthTableControl::depthClampMin) },
            { "param-depthclampmax", make_feild(p.depth_table, &STDepthTableControl::depthClampMax) },
            { "param-disparitymode", make_feild(p.depth_table, &STDepthTableControl::disparityMode) },
            { "param-disparityshift", make_feild(p.depth_table, &STDepthTableControl::disparityShift) },

            // Auto-Exposure
            { "param-autoexposure-setpoint", make_feild(p.ae, &STAEControl::meanIntensitySetPoint) },

            // Census
            { "param-censusenablereg-udiameter", make_feild(p.census, &STCensusRadius::uDiameter) },
            { "param-censusenablereg-vdiameter", make_feild(p.census, &STCensusRadius::vDiameter) },
        };
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
            else
            {
                throw invalid_value_exception(to_string() << "Couldn't parse JSON file! key: " << it.key());
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
