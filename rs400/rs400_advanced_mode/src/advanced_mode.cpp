// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "../../../src/core/advanced_mode.h"

namespace librealsense
{
    ds5_advanced_mode_base::ds5_advanced_mode_base(std::shared_ptr<hw_monitor> hwm, uvc_sensor& depth_sensor)
        : _hw_monitor(hwm),
          _depth_sensor(depth_sensor)
    {
        _enabled = [this](){
            auto results = send_receive(encode_command(ds::fw_cmd::advanced_mode_enabled));
            assert_no_error(ds::fw_cmd::advanced_mode_enabled, results);
            return *(reinterpret_cast<uint32_t*>(results.data()) + 1) > 0;
        };
        _depth_sensor.register_option(RS2_OPTION_ADVANCED_MODE_PRESET,
                                      std::make_shared<advanced_mode_preset_option>(*this,
                                                                                    _depth_sensor,
                                                                                    option_range{0,
                                                                                                 RS2_RS400_VISUAL_PRESET_COUNT - 1,
                                                                                                 1,
                                                                                                 0},
                                                                                    _description_per_value));
    }

    bool ds5_advanced_mode_base::is_enabled() const
    {
        return *_enabled;
    }

    void ds5_advanced_mode_base::toggle_advanced_mode(bool enable)
    {
        send_receive(encode_command(ds::fw_cmd::enable_advanced_mode, enable));
        send_receive(encode_command(ds::fw_cmd::reset));
    }

    std::string ds5_advanced_mode_base::pid_to_str(uint16_t pid)
    {
        return std::string(hexify(pid>>8) + hexify(static_cast<uint8_t>(pid)));
    }


    ds5_advanced_mode_base::res_type ds5_advanced_mode_base::get_res_type(uint32_t width, uint32_t height)
    {
        if (width == 640 && height == 480)
            return res_type::vga_resolution;
        else if (width < 640 && height < 480)
            return res_type::small_resolution;
        else if (width > 640 && height > 480)
            return res_type::full_resolution;

        throw invalid_value_exception(to_string() << "Invalid resolution! " << width << "x" << height);
    }

    void ds5_advanced_mode_base::apply_preset(const std::string& pid,
                      const std::vector<platform::stream_profile>& configuration,
                      rs2_rs400_visual_preset preset)
    {
        auto p = get_all();

        auto res = get_res_type(configuration.front().width, configuration.front().height);

        switch (preset) {
        case RS2_RS400_VISUAL_PRESET_GENERIC_DEPTH: // 5/generic
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                switch (res)
                {
                case small_resolution:
                    generic_ds5_passive_small_seed_config(p);
                    break;
                case vga_resolution:
                    generic_ds5_passive_vga_seed_config(p);
                    break;
                case full_resolution:
                    generic_ds5_passive_full_seed_config(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS400/RS400_MM doesn't support GENERIC_DEPTH preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case vga_resolution:
                    generic_ds5_active_vga_generic_depth(p);
                    break;
                case full_resolution:
                    generic_ds5_active_full_generic_depth(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support GENERIC_DEPTH preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support GENERIC_DEPTH preset!");
            }
            else
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_GENERIC_DEPTH) Failed! Invalid PID! " << pid);
            break;
        case RS2_RS400_VISUAL_PRESET_GENERIC_ACCURATE_DEPTH: // 25
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS400/RS400_MM doesn't support GENERIC_ACCURATE_DEPTH preset!");
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case vga_resolution:
                    generic_ds5_active_vga_accurate_depth(p);
                    break;
                case full_resolution:
                    generic_ds5_active_full_accurate_depth(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support GENERIC_ACCURATE_DEPTH preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                throw invalid_value_exception("RS430_MM doesn't support GENERIC_ACCURATE_DEPTH preset!");
            }
            else
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_GENERIC_ACCURATE_DEPTH) Failed! Invalid PID! " << pid);
            break;
        case RS2_RS400_VISUAL_PRESET_GENERIC_DENSE_DEPTH: // 75
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS400/RS400_MM doesn't support GENERIC_DENSE_DEPTH preset!");
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    generic_ds5_active_small_dense_depth(p);
                    break;
                case vga_resolution:
                    generic_ds5_active_vga_dense_depth(p);
                    break;
                case full_resolution:
                    generic_ds5_active_full_dense_depth(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support GENERIC_DENSE_DEPTH preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                switch (res)
                {
                case small_resolution:
                    generic_ds5_awg_small_dense_depth(p);
                    break;
                case vga_resolution:
                    generic_ds5_awg_vga_dense_depth(p);
                    break;
                case full_resolution:
                    generic_ds5_awg_full_dense_depth(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support GENERIC_DENSE_DEPTH preset with current streaming resolution!");
                }
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_GENERIC_DENSE_DEPTH) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_GENERIC_SUPER_DENSE_DEPTH: // 125
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                throw invalid_value_exception("RS400/RS400_MM doesn't support GENERIC_SUPER_DENSE_DEPTH preset!");
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case full_resolution:
                    generic_ds5_active_full_super_dense_depth(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support GENERIC_SUPER_DENSE_DEPTH preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS430_MM doesn't support GENERIC_SUPER_DENSE_DEPTH preset!");
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_GENERIC_SUPER_DENSE_DEPTH) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_FLOOR_LOW:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS400/RS400_MM doesn't support FLOOR_LOW preset!");
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_active_small_floor_low(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support FLOOR_LOW preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS430_MM doesn't support FLOOR_LOW preset!");
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_FLOOR_LOW) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_3D_BODY_SCAN:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS400/RS400_MM doesn't support 3D_BODY_SCAN preset!");
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case full_resolution:
                    specific_ds5_active_full_3d_body_scan(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support 3D_BODY_SCAN preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                throw invalid_value_exception(to_string() << "apply_preset(...) Failed! RS430_MM doesn't support 3D_BODY_SCAN preset!");
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_3D_BODY_SCAN) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_INDOOR:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_passive_small_indoor_passive(p);
                    break;
                case vga_resolution:
                    specific_ds5_passive_vga_indoor_passive(p);
                    break;
                case full_resolution:
                    specific_ds5_passive_full_indoor_passive(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS400/RS400_MM doesn't support INDOOR preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_active_small_indoor(p);
                    break;
                case vga_resolution:
                    specific_ds5_active_vga_indoor(p);
                    break;
                case full_resolution:
                    specific_ds5_active_full_indoor(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support INDOOR preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_awg_small_indoor(p);
                    break;
                case vga_resolution:
                    specific_ds5_awg_vga_indoor(p);
                    break;
                case full_resolution:
                    specific_ds5_awg_full_indoor(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support INDOOR preset with current streaming resolution!");
                }
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_INDOOR) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_OUTDOOR:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_passive_small_outdoor_passive(p);
                    break;
                case vga_resolution:
                    specific_ds5_passive_vga_outdoor_passive(p);
                    break;
                case full_resolution:
                    specific_ds5_passive_full_outdoor_passive(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS400/RS400_MM doesn't support OUTDOOR preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_active_small_outdoor(p);
                    break;
                case vga_resolution:
                    specific_ds5_active_vga_outdoor(p);
                    break;
                case full_resolution:
                    specific_ds5_active_full_outdoor(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support OUTDOOR preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_awg_small_outdoor(p);
                    break;
                case vga_resolution:
                    specific_ds5_awg_vga_outdoor(p);
                    break;
                case full_resolution:
                    specific_ds5_awg_full_outdoor(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support OUTDOOR preset with current streaming resolution!");
                }
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_OUTDOOR) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_HAND:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                switch (res)
                {
                case small_resolution:
                    specific_ds5_passive_small_hand_gesture(p);
                    break;
                case vga_resolution:
                    specific_ds5_passive_vga_hand_gesture(p);
                    break;
                case full_resolution:
                    specific_ds5_passive_full_hand_gesture(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS400/RS400_MM doesn't support HAND preset with current streaming resolution!");
                }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_active_small_hand_gesture(p);
                    break;
                case vga_resolution:
                    specific_ds5_active_vga_hand_gesture(p);
                    break;
                case full_resolution:
                    specific_ds5_active_full_hand_gesture(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support HAND preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_awg_small_hand_gesture(p);
                    break;
                case vga_resolution:
                    specific_ds5_awg_vga_hand_gesture(p);
                    break;
                case full_resolution:
                    specific_ds5_awg_full_hand_gesture(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support HAND preset with current streaming resolution!");
                }
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_HAND) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_SHORT_RANGE:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_passive_small_face(p);
                    break;
                case vga_resolution:
                    specific_ds5_passive_vga_face(p);
                    break;
                case full_resolution:
                    specific_ds5_passive_full_face(p);

                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS400/RS400_MM doesn't support SHORT_RANGE preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_active_small_face(p);
                    break;
                case vga_resolution:
                    specific_ds5_active_vga_face(p);
                    break;
                case full_resolution:
                    specific_ds5_active_full_face(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support SHORT_RANGE preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_awg_small_face(p);
                    break;
                case vga_resolution:
                    specific_ds5_awg_vga_face(p);
                    break;
                case full_resolution:
                    specific_ds5_awg_full_face(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support SHORT_RANGE preset with current streaming resolution!");
                }
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_SHORT_RANGE) Failed! Invalid PID! " << pid);
            }
            break;
        case RS2_RS400_VISUAL_PRESET_BOX:
            if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_passive_small_box(p);
                    break;
                case vga_resolution:
                    specific_ds5_passive_vga_box(p);
                    break;
                case full_resolution:
                    specific_ds5_passive_full_box(p);

                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS400/RS400_MM doesn't support BOX preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_active_small_box(p);
                    break;
                case vga_resolution:
                    specific_ds5_active_vga_box(p);
                    break;
                case full_resolution:
                    specific_ds5_active_full_box(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS410/RS410_MM doesn't support BOX preset with current streaming resolution!");
                }
            }
            else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
            {
                switch (res)
                {
                case small_resolution:
                    specific_ds5_awg_small_box(p);
                    break;
                case vga_resolution:
                    specific_ds5_awg_vga_box(p);
                    break;
                case full_resolution:
                    specific_ds5_awg_full_box(p);
                    break;
                default:
                    throw invalid_value_exception("apply_preset(...) Failed! RS430_MM doesn't support BOX preset with current streaming resolution!");
                }
            }
            else
            {
                throw invalid_value_exception(to_string() << "apply_preset(RS2_RS400_VISUAL_PRESET_BOX) Failed! Invalid PID! " << pid);
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

    preset ds5_advanced_mode_base::get_all()
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

        auto cur_index = 2;
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
                                                             uvc_sensor& ep, const option_range& opt_range,
                                                             const std::map<float, std::string>& description_per_value)
        : option_base(opt_range),
          _ep(ep),
          _description_per_value(description_per_value),
          _advanced(advanced)
    {}

    rs2_rs400_visual_preset advanced_mode_preset_option::to_preset(float x)
    {
        return (rs2_rs400_visual_preset)((int)x);
    }

    void advanced_mode_preset_option::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception(to_string() << "set(advanced_mode_preset_option) failed! Given value " << value << " is out of range.");

        if (!_advanced.is_enabled())
            throw wrong_api_call_sequence_exception(to_string() << "set(advanced_mode_preset_option) failed! Device is not is Advanced-Mode.");

        if (!_ep.is_streaming())
            throw wrong_api_call_sequence_exception(to_string() << "set(advanced_mode_preset_option) failed! Device must streaming in order to set a preset.");

        auto pid = _ep.get_device().get_info(RS2_CAMERA_INFO_PRODUCT_ID);
        auto configurations = _ep.get_curr_configurations();

        _advanced.apply_preset(pid, configurations, to_preset(value));
        _last_preset = to_preset(value);
    }

    float advanced_mode_preset_option::query() const
    {
        if (!_advanced.is_enabled())
            throw wrong_api_call_sequence_exception(to_string() << "set(advanced_mode_preset_option) failed! Device is not is Advanced-Mode.");

        return static_cast<float>(_last_preset);
    }

    bool advanced_mode_preset_option::is_enabled() const
    {
        return _advanced.is_enabled();
    }

    const char* advanced_mode_preset_option::get_description() const
    {
        return "Advanced-Mode Preset";
    }

    const char* advanced_mode_preset_option::get_value_description(float val) const
    {
        try{
            return _description_per_value.at(val).c_str();
        }
        catch(std::out_of_range)
        {
            throw invalid_value_exception(to_string() << "advanced_mode_preset: get_value_description(...) failed! Description of value " << val << " is not found.");
        }
    }
}
