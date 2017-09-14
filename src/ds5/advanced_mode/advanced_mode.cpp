// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "core/advanced_mode.h"
#include "ds5/ds5-active.h"
#include "ds5/ds5-rolling-shutter.h"
#include "json_loader.hpp"
#include "ds5/ds5-color.h"

namespace librealsense
{
    ds5_advanced_mode_base::ds5_advanced_mode_base(std::shared_ptr<hw_monitor> hwm,
                                                   uvc_sensor& depth_sensor)
        : _hw_monitor(hwm),
          _depth_sensor(depth_sensor),
          _color_sensor(nullptr)
    {
        _enabled = [this]() {
            auto results = send_receive(encode_command(ds::fw_cmd::UAMG));
            assert_no_error(ds::fw_cmd::UAMG, results);
            return results[4] > 0;
        };
        _depth_sensor.register_option(RS2_OPTION_VISUAL_PRESET,
            std::make_shared<advanced_mode_preset_option>(*this,
                                                          _depth_sensor,
                                                          option_range{ 0,
                                                                        RS2_RS400_VISUAL_PRESET_COUNT - 1,
                                                                        1,
                                                                        RS2_RS400_VISUAL_PRESET_CUSTOM }));
        _color_sensor = [this]() {
            auto& dev = _depth_sensor.get_device();
            for (size_t i = 0; i < dev.get_sensors_count(); ++i)
            {
                if (auto s = dynamic_cast<const ds5_color_sensor*>(&(dev.get_sensor(i))))
                {
                    return const_cast<ds5_color_sensor*>(s);
                }
            }
            return (ds5_color_sensor*)nullptr;
        };
    }

    bool ds5_advanced_mode_base::is_enabled() const
    {
        return *_enabled;
    }

    void ds5_advanced_mode_base::toggle_advanced_mode(bool enable)
    {
        send_receive(encode_command(ds::fw_cmd::EN_ADV, enable));
        send_receive(encode_command(ds::fw_cmd::HWRST));
    }

    void ds5_advanced_mode_base::apply_preset(const std::vector<platform::stream_profile>& configuration,
                                              rs2_rs400_visual_preset preset)
    {
        auto p = get_all();
        auto res = get_res_type(configuration.front().width, configuration.front().height);

        switch (preset)
        {
        case RS2_RS400_VISUAL_PRESET_HAND:
            hand_gesture(p);
            break;
        case RS2_RS400_VISUAL_PRESET_SHORT_RANGE:
            short_range(p);
            break;
        case RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY:
            switch (res)
            {
            case low_resolution:
                low_res_high_accuracy(p);
                break;
            case medium_resolution:
                mid_res_high_accuracy(p);
                break;
            case high_resolution:
                high_res_high_accuracy(p);
                break;
            }
            break;
        case RS2_RS400_VISUAL_PRESET_HIGH_DENSITY:
            switch (res)
            {
            case low_resolution:
                low_res_high_density(p);
                break;
            case medium_resolution:
                mid_res_high_density(p);
                break;
            case high_resolution:
                high_res_high_density(p);
                break;
            }
            break;
        case RS2_RS400_VISUAL_PRESET_MEDIUM_DENSITY:
            switch (res)
            {
            case low_resolution:
                low_res_mid_density(p);
                break;
            case medium_resolution:
                mid_res_mid_density(p);
                break;
            case high_resolution:
                high_res_mid_density(p);
                break;
            }
            break;
        default:
            throw invalid_value_exception(to_string() << "Invalid preset! " << preset);
        }
        set_all(p);
    }

    void ds5_advanced_mode_base::get_depth_control_group(STDepthControlGroup* ptr, int mode) const
    {
        *ptr = get<STDepthControlGroup>(advanced_mode_traits<STDepthControlGroup>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_rsm(STRsm* ptr, int mode) const
    {
        *ptr = get<STRsm>(advanced_mode_traits<STRsm>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode) const
    {
        *ptr = get<STRauSupportVectorControl>(advanced_mode_traits<STRauSupportVectorControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_color_control(STColorControl* ptr, int mode) const
    {
        *ptr = get<STColorControl>(advanced_mode_traits<STColorControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode) const
    {
        *ptr = get<STRauColorThresholdsControl>(advanced_mode_traits<STRauColorThresholdsControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode) const
    {
        *ptr = get<STSloColorThresholdsControl>(advanced_mode_traits<STSloColorThresholdsControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_slo_penalty_control(STSloPenaltyControl* ptr, int mode) const
    {
        *ptr = get<STSloPenaltyControl>(advanced_mode_traits<STSloPenaltyControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_hdad(STHdad* ptr, int mode) const
    {
        *ptr = get<STHdad>(advanced_mode_traits<STHdad>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_color_correction(STColorCorrection* ptr, int mode) const
    {
        *ptr = get<STColorCorrection>(advanced_mode_traits<STColorCorrection>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_depth_table_control(STDepthTableControl* ptr, int mode) const
    {
        *ptr = get<STDepthTableControl>(advanced_mode_traits<STDepthTableControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_ae_control(STAEControl* ptr, int mode) const
    {
        *ptr = get<STAEControl>(advanced_mode_traits<STAEControl>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_census_radius(STCensusRadius* ptr, int mode) const
    {
        *ptr = get<STCensusRadius>(advanced_mode_traits<STCensusRadius>::group, nullptr, mode);
    }

    void ds5_advanced_mode_base::get_laser_power(laser_power_control* ptr) const
    {
        ptr->laser_power = _depth_sensor.get_option(RS2_OPTION_LASER_POWER).query();
        ptr->was_set = true;
    }

    void ds5_advanced_mode_base::get_laser_state(laser_state_control* ptr) const
    {
        ptr->laser_state = static_cast<int>(_depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED).query());
        ptr->was_set = true;
    }

    void ds5_advanced_mode_base::get_exposure(uvc_sensor& sensor, exposure_control* ptr) const
    {
        ptr->exposure = sensor.get_option(RS2_OPTION_EXPOSURE).query();
        ptr->was_set = true;
    }

    void ds5_advanced_mode_base::get_auto_exposure(uvc_sensor& sensor, auto_exposure_control* ptr) const
    {
        ptr->auto_exposure = static_cast<int>(sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).query());
        ptr->was_set = true;
    }

    void ds5_advanced_mode_base::get_depth_exposure(exposure_control* ptr) const
    {
        get_exposure(_depth_sensor, ptr);
    }

    void ds5_advanced_mode_base::get_depth_auto_exposure(auto_exposure_control* ptr) const
    {
        get_auto_exposure(_depth_sensor, ptr);
    }

    void ds5_advanced_mode_base::get_depth_gain(gain_control* ptr) const
    {
        ptr->gain = _depth_sensor.get_option(RS2_OPTION_GAIN).query();
        ptr->was_set = true;
    }

    void ds5_advanced_mode_base::get_color_exposure(exposure_control* ptr) const
    {
        if (*_color_sensor)
        {
            get_exposure(*(*_color_sensor), ptr);
        }
    }

    void ds5_advanced_mode_base::get_color_auto_exposure(auto_exposure_control* ptr) const
    {
        if (*_color_sensor)
        {
            get_auto_exposure(**_color_sensor, ptr);
        }
    }

    void ds5_advanced_mode_base::get_color_backlight_compensation(backlight_compensation_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->backlight_compensation = static_cast<int>((*_color_sensor)->get_option(RS2_OPTION_BACKLIGHT_COMPENSATION).query());
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_brightness(brightness_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->brightness = (*_color_sensor)->get_option(RS2_OPTION_BRIGHTNESS).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_contrast(contrast_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->contrast = (*_color_sensor)->get_option(RS2_OPTION_CONTRAST).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_gain(gain_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->gain = (*_color_sensor)->get_option(RS2_OPTION_GAIN).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_gamma(gamma_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->gamma = (*_color_sensor)->get_option(RS2_OPTION_GAMMA).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_hue(hue_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->hue = (*_color_sensor)->get_option(RS2_OPTION_HUE).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_saturation(saturation_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->saturation = (*_color_sensor)->get_option(RS2_OPTION_SATURATION).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_sharpness(sharpness_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->sharpness = (*_color_sensor)->get_option(RS2_OPTION_SHARPNESS).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_white_balance(white_balance_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->white_balance = (*_color_sensor)->get_option(RS2_OPTION_WHITE_BALANCE).query();
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_auto_white_balance(auto_white_balance_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->auto_white_balance = static_cast<int>((*_color_sensor)->get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE).query());
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::get_color_power_line_frequency(power_line_frequency_control* ptr) const
    {
        if (*_color_sensor)
        {
            ptr->power_line_frequency = static_cast<int>((*_color_sensor)->get_option(RS2_OPTION_POWER_LINE_FREQUENCY).query());
            ptr->was_set = true;
        }
    }

    void ds5_advanced_mode_base::set_depth_control_group(const STDepthControlGroup& val)
    {
        set(val, advanced_mode_traits<STDepthControlGroup>::group);
    }

    void ds5_advanced_mode_base::set_rsm(const STRsm& val)
    {
        set(val, advanced_mode_traits<STRsm>::group);
    }

    void ds5_advanced_mode_base::set_rau_support_vector_control(const STRauSupportVectorControl& val)
    {
        set(val, advanced_mode_traits<STRauSupportVectorControl>::group);
    }

    void ds5_advanced_mode_base::set_color_control(const STColorControl& val)
    {
        set(val, advanced_mode_traits<STColorControl>::group);
    }

    void ds5_advanced_mode_base::set_rau_color_thresholds_control(const STRauColorThresholdsControl& val)
    {
        set(val, advanced_mode_traits<STRauColorThresholdsControl>::group);
    }

    void ds5_advanced_mode_base::set_slo_color_thresholds_control(const STSloColorThresholdsControl& val)
    {
        set(val, advanced_mode_traits<STSloColorThresholdsControl>::group);
    }

    void ds5_advanced_mode_base::set_slo_penalty_control(const STSloPenaltyControl& val)
    {
        set(val, advanced_mode_traits<STSloPenaltyControl>::group);
    }

    void ds5_advanced_mode_base::set_hdad(const STHdad& val)
    {
        set(val, advanced_mode_traits<STHdad>::group);
    }

    void ds5_advanced_mode_base::set_color_correction(const STColorCorrection& val)
    {
        set(val, advanced_mode_traits<STColorCorrection>::group);
    }

    void ds5_advanced_mode_base::set_depth_table_control(const STDepthTableControl& val)
    {
        set(val, advanced_mode_traits<STDepthTableControl>::group);
    }

    void ds5_advanced_mode_base::set_ae_control(const STAEControl& val)
    {
        set(val, advanced_mode_traits<STAEControl>::group);
    }

    void ds5_advanced_mode_base::set_census_radius(const STCensusRadius& val)
    {
        set(val, advanced_mode_traits<STCensusRadius>::group);
    }

    void ds5_advanced_mode_base::set_laser_power(const laser_power_control& val)
    {
        if (val.was_set)
            _depth_sensor.get_option(RS2_OPTION_LASER_POWER).set(val.laser_power);
    }

    void ds5_advanced_mode_base::set_laser_state(const laser_state_control& val)
    {
        if (val.was_set)
            _depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED).set(val.laser_state);
    }

    void ds5_advanced_mode_base::set_exposure(uvc_sensor& sensor, const exposure_control& val)
    {
        sensor.get_option(RS2_OPTION_EXPOSURE).set(val.exposure);
    }

    void ds5_advanced_mode_base::set_auto_exposure(uvc_sensor& sensor, const auto_exposure_control& val)
    {
        sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE).set(val.auto_exposure);
    }

    void ds5_advanced_mode_base::set_depth_exposure(const exposure_control& val)
    {
        if (val.was_set)
            set_exposure(_depth_sensor, val);
    }

    void ds5_advanced_mode_base::set_depth_auto_exposure(const auto_exposure_control& val)
    {
        if (val.was_set)
            set_auto_exposure(_depth_sensor, val);
    }

    void ds5_advanced_mode_base::set_depth_gain(const gain_control& val)
    {
        if (val.was_set)
            _depth_sensor.get_option(RS2_OPTION_GAIN).set(val.gain);
    }

    void ds5_advanced_mode_base::set_color_exposure(const exposure_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_exposure value! Color sensor not found.");

        if (val.was_set)
            set_exposure(**_color_sensor, val);
    }

    void ds5_advanced_mode_base::set_color_auto_exposure(const auto_exposure_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_auto_exposure value! Color sensor not found.");

        if (val.was_set)
            set_auto_exposure(**_color_sensor, val);
    }

    void ds5_advanced_mode_base::set_color_backlight_compensation(const backlight_compensation_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_backlight_compensation value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_BACKLIGHT_COMPENSATION).set(val.backlight_compensation);
    }

    void ds5_advanced_mode_base::set_color_brightness(const brightness_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_brightness value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_BRIGHTNESS).set(val.brightness);
    }

    void ds5_advanced_mode_base::set_color_contrast(const contrast_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_contrast value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_CONTRAST).set(val.contrast);
    }

    void ds5_advanced_mode_base::set_color_gain(const gain_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_gain value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_GAIN).set(val.gain);
    }

    void ds5_advanced_mode_base::set_color_gamma(const gamma_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_gamma value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_GAMMA).set(val.gamma);
    }

    void ds5_advanced_mode_base::set_color_hue(const hue_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_hue value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_HUE).set(val.hue);
    }

    void ds5_advanced_mode_base::set_color_saturation(const saturation_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_saturation value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_SATURATION).set(val.saturation);
    }

    void ds5_advanced_mode_base::set_color_sharpness(const sharpness_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_sharpness value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_SHARPNESS).set(val.sharpness);
    }

    void ds5_advanced_mode_base::set_color_white_balance(const white_balance_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_white_balance value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_WHITE_BALANCE).set(val.white_balance);
    }

    void ds5_advanced_mode_base::set_color_auto_white_balance(const auto_white_balance_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_auto_white_balance value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE).set(val.auto_white_balance);
    }

    void ds5_advanced_mode_base::set_color_power_line_frequency(const power_line_frequency_control& val)
    {
        if (val.was_set && !*_color_sensor)
            throw invalid_value_exception("Can't set color_power_line_frequency value! Color sensor not found.");

        if (val.was_set)
            (*_color_sensor)->get_option(RS2_OPTION_POWER_LINE_FREQUENCY).set(val.power_line_frequency);
    }

    std::vector<uint8_t> ds5_advanced_mode_base::serialize_json() const
    {
        auto p = get_all();
        return generate_json(p);
    }

    void ds5_advanced_mode_base::load_json(const std::string& json_content)
    {
        auto p = get_all();
        update_structs(json_content, p);
        set_all(p);
    }

    preset ds5_advanced_mode_base::get_all() const
    {
        preset p;
        get_depth_control_group(&p.depth_controls);
        get_rsm(&p.rsm);
        get_rau_support_vector_control(&p.rsvc);
        get_color_control(&p.color_control);
        get_rau_color_thresholds_control(&p.rctc);
        get_slo_color_thresholds_control(&p.sctc);
        get_slo_penalty_control(&p.spc);
        get_hdad(&p.hdad);
        get_color_correction(&p.cc);
        get_depth_table_control(&p.depth_table);
        get_ae_control(&p.ae);
        get_census_radius(&p.census);
        get_laser_power(&p.laser_power);
        get_laser_state(&p.laser_state);
        get_depth_exposure(&p.depth_exposure);
        get_depth_auto_exposure(&p.depth_auto_exposure);
        get_depth_gain(&p.depth_gain);
        get_color_exposure(&p.color_exposure);
        get_color_auto_exposure(&p.color_auto_exposure);
        get_color_backlight_compensation(&p.color_backlight_compensation);
        get_color_brightness(&p.color_brightness);
        get_color_contrast(&p.color_contrast);
        get_color_gain(&p.color_gain);
        get_color_gamma(&p.color_gamma);
        get_color_hue(&p.color_hue);
        get_color_saturation(&p.color_saturation);
        get_color_sharpness(&p.color_sharpness);
        get_color_white_balance(&p.color_white_balance);
        get_color_auto_white_balance(&p.color_auto_white_balance);
        get_color_power_line_frequency(&p.color_power_line_frequency);
        return p;
    }

    void ds5_advanced_mode_base::set_all(const preset& p)
    {
        set_depth_control_group(p.depth_controls);
        set_rsm(p.rsm);
        set_rau_support_vector_control(p.rsvc);
        set_color_control(p.color_control);
        set_rau_color_thresholds_control(p.rctc);
        set_slo_color_thresholds_control(p.sctc);
        set_slo_penalty_control(p.spc);
        set_hdad(p.hdad);
        set_color_correction(p.cc);
        set_depth_table_control(p.depth_table);
        set_ae_control(p.ae);
        set_census_radius(p.census);

        set_laser_state(p.laser_state);
        if (p.laser_state.was_set && p.laser_state.laser_state == 1) // 1 - on
            set_laser_power(p.laser_power);

        set_depth_auto_exposure(p.depth_auto_exposure);
        if (p.depth_auto_exposure.was_set && p.depth_auto_exposure.auto_exposure == 0)
        {
            set_depth_gain(p.depth_gain);
            set_depth_exposure(p.depth_exposure);
        }

        set_color_auto_exposure(p.color_auto_exposure);
        if (p.color_auto_exposure.was_set && p.color_auto_exposure.auto_exposure == 0)
            set_color_exposure(p.color_exposure);

        set_color_backlight_compensation(p.color_backlight_compensation);
        set_color_brightness(p.color_brightness);
        set_color_contrast(p.color_contrast);
        set_color_gain(p.color_gain);
        set_color_gamma(p.color_gamma);
        set_color_hue(p.color_hue);
        set_color_saturation(p.color_saturation);
        set_color_sharpness(p.color_sharpness);

        set_color_auto_white_balance(p.color_auto_white_balance);
        if (p.color_auto_white_balance.was_set && p.color_auto_white_balance.auto_white_balance == 0)
            set_color_white_balance(p.color_white_balance);

        // TODO: Itay, check the issue of setting PWF to auto
        //set_color_power_line_frequency(p.color_power_line_frequency);
    }

    std::vector<uint8_t> ds5_advanced_mode_base::send_receive(const std::vector<uint8_t>& input) const
    {
        auto res = _hw_monitor->send(input);
        if (res.empty())
        {
            throw std::runtime_error("Advanced mode write failed!");
        }
        return res;
    }

    uint32_t ds5_advanced_mode_base::pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
    {
        return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
    }

    std::vector<uint8_t> ds5_advanced_mode_base::assert_no_error(ds::fw_cmd opcode, const std::vector<uint8_t>& results)
    {
        if (results.size() < sizeof(uint32_t)) throw std::runtime_error("Incomplete operation result!");
        auto opCodeAsUint32 = pack(results[3], results[2], results[1], results[0]);
        if (opCodeAsUint32 != static_cast<uint32_t>(opcode))
        {
            std::stringstream ss;
            ss << "Operation failed with error code=" << static_cast<int>(opCodeAsUint32);
            throw std::runtime_error(ss.str());
        }
        std::vector<uint8_t> result;
        result.resize(results.size() - sizeof(uint32_t));
        std::copy(results.data() + sizeof(uint32_t),
            results.data() + results.size(), result.data());
        return result;
    }

    std::vector<uint8_t> ds5_advanced_mode_base::encode_command(ds::fw_cmd opcode,
        uint32_t p1,
        uint32_t p2,
        uint32_t p3,
        uint32_t p4,
        std::vector<uint8_t> data) const
    {
        std::vector<uint8_t> raw_data;
        auto cmd_op_code = static_cast<uint32_t>(opcode);

        const uint16_t pre_header_data = 0xcdab;
        raw_data.resize(HW_MONITOR_BUFFER_SIZE);
        auto write_ptr = raw_data.data();
        auto header_size = 4;

        size_t cur_index = 2;
        *reinterpret_cast<uint16_t *>(write_ptr + cur_index) = pre_header_data;
        cur_index += sizeof(uint16_t);
        *reinterpret_cast<unsigned int *>(write_ptr + cur_index) = cmd_op_code;
        cur_index += sizeof(unsigned int);

        // Parameters
        *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p1;
        cur_index += sizeof(unsigned);
        *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p2;
        cur_index += sizeof(unsigned);
        *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p3;
        cur_index += sizeof(unsigned);
        *reinterpret_cast<unsigned*>(write_ptr + cur_index) = p4;
        cur_index += sizeof(unsigned);

        // Data
        std::copy(data.begin(), data.end(), reinterpret_cast<uint8_t*>(write_ptr + cur_index));
        cur_index += data.size();

        *reinterpret_cast<uint16_t*>(raw_data.data()) = static_cast<uint16_t>(cur_index - header_size);// Length doesn't include hdr.
        raw_data.resize(cur_index);
        return raw_data;
    }

    advanced_mode_preset_option::advanced_mode_preset_option(ds5_advanced_mode_base& advanced,
        uvc_sensor& ep, const option_range& opt_range)
        : option_base(opt_range),
        _ep(ep),
        _advanced(advanced),
        _last_preset(RS2_RS400_VISUAL_PRESET_CUSTOM)
    {
        _ep.register_on_open([this](std::vector<platform::stream_profile> configurations) {
            std::lock_guard<std::mutex> lock(_mtx);
            if (_last_preset != RS2_RS400_VISUAL_PRESET_CUSTOM)
                _advanced.apply_preset(configurations, _last_preset);
        });
    }

    rs2_rs400_visual_preset advanced_mode_preset_option::to_preset(float x)
    {
        return (static_cast<rs2_rs400_visual_preset>((int)x));
    }

    void advanced_mode_preset_option::set(float value)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (!is_valid(value))
            throw invalid_value_exception(to_string() << "set(advanced_mode_preset_option) failed! Given value " << value << " is out of range.");

        if (!_advanced.is_enabled())
            throw wrong_api_call_sequence_exception(to_string() << "set(advanced_mode_preset_option) failed! Device is not in Advanced-Mode.");

        auto preset = to_preset(value);
        if (preset == RS2_RS400_VISUAL_PRESET_CUSTOM || !_ep.is_streaming())
        {
            _last_preset = preset;
            return;
        }

        auto uvc_sensor = dynamic_cast<librealsense::uvc_sensor*>(&_ep);

        auto configurations = uvc_sensor->get_configuration();
        _advanced.apply_preset(configurations, preset);
        _last_preset = preset;
    }

    float advanced_mode_preset_option::query() const
    {
        return static_cast<float>(_last_preset);
    }

    bool advanced_mode_preset_option::is_enabled() const
    {
        return true;
    }

    const char* advanced_mode_preset_option::get_description() const
    {
        return "Advanced-Mode Preset";
    }

    const char* advanced_mode_preset_option::get_value_description(float val) const
    {
        try {
            return rs2_rs400_visual_preset_to_string(to_preset(val));
        }
        catch (std::out_of_range)
        {
            throw invalid_value_exception(to_string() << "advanced_mode_preset: get_value_description(...) failed! Description of value " << val << " is not found.");
        }
    }
}
