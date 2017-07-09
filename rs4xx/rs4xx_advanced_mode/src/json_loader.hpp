/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

// This sample uses the advanced mode to set the algo controls of DS5.
// It can be useful for loading settings saved from IPDev.

#include <fstream>

#include <chrono>
#include <thread>
#include <sstream>
#include <map>
#include <string>

#include "../third-party/json.hpp"
#include "rs4xx_advanced_mode.hpp"

using json = nlohmann::json;

template<class T>
struct param_group
{
    using group_type = T;
    T vals[3];
    bool update = false;
};

struct stream_profile
{
    int width;
    int height;
    int fps;
    std::string depth_format;
    std::string ir_format;
};

struct laser_control
{
    std::string laser_state;
    int laser_power;
};

struct roi_control
{
    int roi_top;
    int roi_bottom;
    int roi_left;
    int roi_right;
};

struct exposure_control
{
    std::string auto_exposure;
    int exposure;
};

struct camera_state
{
    param_group<::STDepthControlGroup> depth_controls;
    param_group<::STRsm> rsm;
    param_group<::STRauSupportVectorControl> rsvc;
    param_group<::STColorControl> color_control;
    param_group<::STRauColorThresholdsControl> rctc;
    param_group<::STSloColorThresholdsControl> sctc;
    param_group<::STSloPenaltyControl> spc;
    param_group<::STHdad> hdad;
    param_group<::STColorCorrection> cc;
    param_group<::STDepthTableControl> depth_table;
    param_group<::STAEControl> ae;
    param_group<::STCensusRadius> census;
    param_group<stream_profile> profile;
    param_group<laser_control> laser;
    param_group<roi_control> roi;
    param_group<exposure_control> exposure;

    void reset()
    {
        depth_controls.update = false;
        rsm.update = false;
        rsvc.update = false;
        color_control.update = false;
        rctc.update = false;
        sctc.update = false;
        spc.update = false;
        hdad.update = false;
        cc.update = false;
        depth_table.update = false;
        ae.update = false;
        census.update = false;
        profile.update = false;
        laser.update = false;
        roi.update = false;
        exposure.update = false;

        profile.vals[0].depth_format = "Z16";
        profile.vals[0].ir_format = "COUNT";
    }
};

struct json_feild
{
    virtual ~json_feild() = default;

    bool was_set = false;

    virtual void load(const std::string& value) = 0;
};

template<class T, class S>
struct json_struct_feild : json_feild
{
    T* strct;
    S T::group_type::* field;
    float scale = 1.0f;
    bool check_ranges = true;

    void range_check(float value)
    {
        if (check_ranges)
        {
            auto min = ((strct->vals + 1)->*field);
            auto max = ((strct->vals + 2)->*field);
            if (value > max || value < min)
            {
                std::stringstream ss;
                ss << "value " << value << " is not in range [" << min << ", " << max << "]!";
                throw std::runtime_error(ss.str());
            }
        }
    }
    
    void load(const std::string& str) override
    {
        float value = ::atof(str.c_str());
        range_check(scale * value);
        (strct->vals[0].*field) = scale * value;
        strct->update = true;
    }
};

template<class T, class S>
struct json_string_struct_feild : json_feild
{
    T* strct;
    S T::group_type::* field;

    void load(const std::string& value) override
    {
        (strct->vals[0].*field) = value;
        strct->update = true;
    }
};

template<class T, class S>
struct json_invert_struct_feild : json_struct_feild<T, S>
{
    using json_struct_feild<T, S>::range_check;
    using json_struct_feild<T, S>::strct;
    using json_struct_feild<T, S>::field;

    void load(const std::string& str) override
    {
        float value = ::atof(str.c_str());
        range_check(value);
        (strct->vals[0].*field) = (value > 0) ? 0 : 1;
        strct->update = true;
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
std::shared_ptr<json_feild> make_string_feild(T& strct, S T::group_type::* feild, double scale = 1.0f)
{
    std::shared_ptr<json_string_struct_feild<T, S>> f(new json_string_struct_feild<T, S>());
    f->field = feild;
    f->strct = &strct;
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


inline void load_json(const std::string& filename, camera_state& state)
{
    std::stringstream errors;
    std::stringstream warnings;

    // Read JSON file
    std::ifstream i(filename);
    json j;
    i >> j;

    std::map<std::string, std::shared_ptr<json_feild>> feilds = {
        // Depth Control
        { "param-leftrightthreshold", make_feild(state.depth_controls, &STDepthControlGroup::lrAgreeThreshold) },
        { "param-maxscorethreshb", make_feild(state.depth_controls, &STDepthControlGroup::scoreThreshB) },
        { "param-medianthreshold", make_feild(state.depth_controls, &STDepthControlGroup::deepSeaMedianThreshold) },
        { "param-minscorethresha", make_feild(state.depth_controls, &STDepthControlGroup::scoreThreshA) },
        { "param-neighborthresh", make_feild(state.depth_controls, &STDepthControlGroup::deepSeaNeighborThreshold) },
        { "param-secondpeakdelta", make_feild(state.depth_controls, &STDepthControlGroup::deepSeaSecondPeakThreshold) },
        { "param-texturecountthresh", make_feild(state.depth_controls, &STDepthControlGroup::textureCountThreshold) },
        { "param-robbinsmonrodecrement", make_feild(state.depth_controls, &STDepthControlGroup::minusDecrement) },
        { "param-robbinsmonroincrement", make_feild(state.depth_controls, &STDepthControlGroup::plusIncrement) },

        // RSM
        { "param-usersm", make_invert_feild(state.rsm, &STRsm::rsmBypass) },
        { "param-rsmdiffthreshold", make_feild(state.rsm, &STRsm::diffThresh) },
        { "param-rsmrauslodiffthreshold", make_feild(state.rsm, &STRsm::sloRauDiffThresh) },
        { "param-rsmremovethreshold", make_feild(state.rsm, &STRsm::removeThresh, 168) },

        // RAU Support Vector Control
        { "param-raumine", make_feild(state.rsvc, &STRauSupportVectorControl::minEast) },
        { "param-rauminn", make_feild(state.rsvc, &STRauSupportVectorControl::minNorth) },
        { "param-rauminnssum", make_feild(state.rsvc, &STRauSupportVectorControl::minNSsum) },
        { "param-raumins", make_feild(state.rsvc, &STRauSupportVectorControl::minSouth) },
        { "param-rauminw", make_feild(state.rsvc, &STRauSupportVectorControl::minWest) },
        { "param-rauminwesum", make_feild(state.rsvc, &STRauSupportVectorControl::minWEsum) },
        { "param-regionshrinku", make_feild(state.rsvc, &STRauSupportVectorControl::uShrink) },
        { "param-regionshrinkv", make_feild(state.rsvc, &STRauSupportVectorControl::vShrink) },

        // Color Controls
        { "param-disableraucolor", make_feild(state.color_control, &STColorControl::disableRAUColor) },
        { "param-disablesadcolor", make_feild(state.color_control, &STColorControl::disableSADColor) },
        { "param-disablesadnormalize", make_feild(state.color_control, &STColorControl::disableSADNormalize) },
        { "param-disablesloleftcolor", make_feild(state.color_control, &STColorControl::disableSLOLeftColor) },
        { "param-disableslorightcolor", make_feild(state.color_control, &STColorControl::disableSLORightColor) },

        // RAU Color Thresholds Control
        { "param-regioncolorthresholdb", make_feild(state.rctc, &STRauColorThresholdsControl::rauDiffThresholdBlue, 1022) },
        { "param-regioncolorthresholdg", make_feild(state.rctc, &STRauColorThresholdsControl::rauDiffThresholdGreen, 1022) },
        { "param-regioncolorthresholdr", make_feild(state.rctc, &STRauColorThresholdsControl::rauDiffThresholdRed, 1022) },

        // SLO Color Thresholds Control
        { "param-scanlineedgetaub", make_feild(state.sctc, &STSloColorThresholdsControl::diffThresholdBlue) },
        { "param-scanlineedgetaug", make_feild(state.sctc, &STSloColorThresholdsControl::diffThresholdGreen) },
        { "param-scanlineedgetaur", make_feild(state.sctc, &STSloColorThresholdsControl::diffThresholdRed) },

        // SLO Penalty Control
        { "param-scanlinep1", make_feild(state.spc, &STSloPenaltyControl::sloK1Penalty) },
        { "param-scanlinep1onediscon", make_feild(state.spc, &STSloPenaltyControl::sloK1PenaltyMod1) },
        { "param-scanlinep1twodiscon", make_feild(state.spc, &STSloPenaltyControl::sloK1PenaltyMod2) },
        { "param-scanlinep2", make_feild(state.spc, &STSloPenaltyControl::sloK2Penalty) },
        { "param-scanlinep2onediscon", make_feild(state.spc, &STSloPenaltyControl::sloK2PenaltyMod1) },
        { "param-scanlinep2twodiscon", make_feild(state.spc, &STSloPenaltyControl::sloK2PenaltyMod2) },

        // HDAD
        { "param-lambdaad", make_feild(state.hdad, &STHdad::lambdaAD) },
        { "param-lambdacensus", make_feild(state.hdad, &STHdad::lambdaCensus) },
        { "ignoreSAD", make_feild(state.hdad, &STHdad::ignoreSAD) },

        // SLO Penalty Control
        { "param-colorcorrection1", make_feild(state.cc, &STColorCorrection::colorCorrection1) },
        { "param-colorcorrection2", make_feild(state.cc, &STColorCorrection::colorCorrection2) },
        { "param-colorcorrection3", make_feild(state.cc, &STColorCorrection::colorCorrection3) },
        { "param-colorcorrection4", make_feild(state.cc, &STColorCorrection::colorCorrection4) },
        { "param-colorcorrection5", make_feild(state.cc, &STColorCorrection::colorCorrection5) },
        { "param-colorcorrection6", make_feild(state.cc, &STColorCorrection::colorCorrection6) },
        { "param-colorcorrection7", make_feild(state.cc, &STColorCorrection::colorCorrection7) },
        { "param-colorcorrection8", make_feild(state.cc, &STColorCorrection::colorCorrection8) },
        { "param-colorcorrection9", make_feild(state.cc, &STColorCorrection::colorCorrection9) },
        { "param-colorcorrection10", make_feild(state.cc, &STColorCorrection::colorCorrection10) },
        { "param-colorcorrection11", make_feild(state.cc, &STColorCorrection::colorCorrection11) },
        { "param-colorcorrection12", make_feild(state.cc, &STColorCorrection::colorCorrection12) },

        // Depth Table
        { "param-depthunits", make_feild(state.depth_table, &STDepthTableControl::depthUnits) },
        { "param-depthclampmin", make_feild(state.depth_table, &STDepthTableControl::depthClampMin) },
        { "param-depthclampmax", make_feild(state.depth_table, &STDepthTableControl::depthClampMax) },
        { "param-disparitymode", make_feild(state.depth_table, &STDepthTableControl::disparityMode) },
        { "param-disparityshift", make_feild(state.depth_table, &STDepthTableControl::disparityShift) },
        
        // Auto-Exposure
        { "param-autoexposure-setpoint", make_feild(state.ae, &STAEControl::meanIntensitySetPoint) },

        // Census
        { "param-censusenablereg-udiameter", make_feild(state.census, &STCensusRadius::uDiameter) },
        { "param-censusenablereg-vdiameter", make_feild(state.census, &STCensusRadius::vDiameter) },

        // Stream Profile
        { "stream-width", make_feild(state.profile, &stream_profile::width) },
        { "stream-height", make_feild(state.profile, &stream_profile::height) },
        { "stream-fps", make_feild(state.profile, &stream_profile::fps) },
        { "stream-depth-format", make_string_feild(state.profile, &stream_profile::depth_format) },
        { "stream-ir-format", make_string_feild(state.profile, &stream_profile::ir_format) },

        // Controls Group
        { "controls-laserstate", make_string_feild(state.laser, &laser_control::laser_state) },
        { "controls-laserpower", make_feild(state.laser, &laser_control::laser_power) },

        { "controls-autoexposure-roi-top", make_feild(state.roi, &roi_control::roi_top) },
        { "controls-autoexposure-roi-bottom", make_feild(state.roi, &roi_control::roi_bottom) },
        { "controls-autoexposure-roi-left", make_feild(state.roi, &roi_control::roi_left) },
        { "controls-autoexposure-roi-right", make_feild(state.roi, &roi_control::roi_right) },

        { "controls-autoexposure-auto", make_string_feild(state.exposure, &exposure_control::auto_exposure) },
        { "controls-autoexposure-manual", make_feild(state.exposure, &exposure_control::exposure) },
    };

    for (auto it = j.begin(); it != j.end(); ++it)
    {
        auto kvp = feilds.find(it.key());
        if (kvp != feilds.end())
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
            catch (const std::runtime_error& err)
            {
                errors << "Couldn't set \"" << it.key() << "\":" << err.what() << "\n";
            }
        }
    }

    for (auto& kvp : feilds)
    {
        if (!kvp.second->was_set)
        {
            warnings << "(Warning) Parameter \"" << kvp.first << "\" was not set!\n";
        }
    }

    auto err = errors.str();
    if (err != "") throw std::runtime_error(err + warnings.str());
}
