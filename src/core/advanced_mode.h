// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "ds5/ds5-private.h"
#include "hw-monitor.h"
#include "streaming.h"
#include "../option.h"
#define RS4XX_ADVANCED_MODE_HPP
#include "../../rs4xx/rs4xx_advanced_mode/src/presets.h"
#include <librealsense/rs2_advanced_mode_command.h>
#undef RS4XX_ADVANCED_MODE_HPP

typedef enum
{
    etDepthControl              = 0,
    etRsm                       = 1,
    etRauSupportVectorControl   = 2,
    etColorControl              = 3,
    etRauColorThresholdsControl = 4,
    etSloColorThresholdsControl = 5,
    etSloPenaltyControl         = 6,
    etHdad                      = 7,
    etColorCorrection           = 8,
    etDepthTableControl         = 9,
    etAEControl                 = 10,
    etCencusRadius9             = 11,
    etLastAdvancedModeGroup     = 12,       // Must be last
}
EtAdvancedModeRegGroup;

namespace librealsense
{
    template<class T>
    struct advanced_mode_traits;

    #define MAP_ADVANCED_MODE(T, E) template<> struct advanced_mode_traits<T> { static const EtAdvancedModeRegGroup group = E; }

    MAP_ADVANCED_MODE(STDepthControlGroup, etDepthControl);
    MAP_ADVANCED_MODE(STRsm, etRsm);
    MAP_ADVANCED_MODE(STRauSupportVectorControl, etRauSupportVectorControl);
    MAP_ADVANCED_MODE(STColorControl, etColorControl);
    MAP_ADVANCED_MODE(STRauColorThresholdsControl, etRauColorThresholdsControl);
    MAP_ADVANCED_MODE(STSloColorThresholdsControl, etSloColorThresholdsControl);
    MAP_ADVANCED_MODE(STSloPenaltyControl, etSloPenaltyControl);
    MAP_ADVANCED_MODE(STHdad, etHdad);
    MAP_ADVANCED_MODE(STColorCorrection, etColorCorrection);
    MAP_ADVANCED_MODE(STDepthTableControl, etDepthTableControl);
    MAP_ADVANCED_MODE(STAEControl, etAEControl);
    MAP_ADVANCED_MODE(STCensusRadius, etCencusRadius9);


    class ds5_advanced_mode_interface : public virtual device_interface
    {
    public:
        virtual bool is_enabled() const = 0;

        virtual void toggle_advanced_mode(bool enable) = 0;

        virtual void apply_preset(const std::string& pid,
                                  const std::vector<platform::stream_profile>& configuration,
                                  rs2_advanced_mode_preset preset) = 0;

        virtual void get_depth_control_group(STDepthControlGroup* ptr, int mode = 0) const = 0;
        virtual void get_rsm(STRsm* ptr, int mode = 0) const = 0;
        virtual void get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode = 0) const = 0;
        virtual void get_color_control(STColorControl* ptr, int mode = 0) const = 0;
        virtual void get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode = 0) const = 0;
        virtual void get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode = 0) const = 0;
        virtual void get_slo_penalty_control(STSloPenaltyControl* ptr, int mode = 0) const = 0;
        virtual void get_hdad(STHdad* ptr, int mode = 0) const = 0;
        virtual void get_color_correction(STColorCorrection* ptr, int mode = 0) const = 0;
        virtual void get_depth_table_control(STDepthTableControl* ptr, int mode = 0) const = 0;
        virtual void get_ae_control(STAEControl* ptr, int mode = 0) const = 0;
        virtual void get_census_radius(STCensusRadius* ptr, int mode = 0) const = 0;

        virtual void set_depth_control_group(const STDepthControlGroup& val) = 0;
        virtual void set_rsm(const STRsm& val) = 0;
        virtual void set_rau_support_vector_control(const STRauSupportVectorControl& val) = 0;
        virtual void set_color_control(const STColorControl& val) = 0;
        virtual void set_rau_color_thresholds_control(const STRauColorThresholdsControl& val) = 0;
        virtual void set_slo_color_thresholds_control(const STSloColorThresholdsControl& val) = 0;
        virtual void set_slo_penalty_control(const STSloPenaltyControl& val) = 0;
        virtual void set_hdad(const STHdad& val) = 0;
        virtual void set_color_correction(const STColorCorrection& val) = 0;
        virtual void set_depth_table_control(const STDepthTableControl& val) = 0;
        virtual void set_ae_control(const STAEControl& val) = 0;
        virtual void set_census_radius(const STCensusRadius& val) = 0;

        virtual ~ds5_advanced_mode_interface() = default;
    };

    class advanced_mode_preset_option;

    class ds5_advanced_mode_base : public ds5_advanced_mode_interface
    {
    public:
        explicit ds5_advanced_mode_base(std::shared_ptr<hw_monitor> hwm, uvc_sensor& depth_sensor);

        bool is_enabled() const
        {
            return *_enabled;
        }

        void toggle_advanced_mode(bool enable)
        {
            send_recieve(encode_command(ds::fw_cmd::enable_advanced_mode, enable));
            send_recieve(encode_command(ds::fw_cmd::reset));
        }

        std::string pid_to_str(uint16_t pid)
        {
            return std::string(hexify(pid>>8) + hexify(static_cast<uint8_t>(pid)));
        }

        enum res_type{
            low_resolution,
            vga_resolution,
            high_resolution
        };

        res_type get_res_type(uint32_t width, uint32_t height)
        {
            if (width == 640 && height == 480)
                return res_type::vga_resolution;
            else if (width < 640 && height < 480)
                return res_type::low_resolution;
            else if (width > 640 && height > 480)
                return res_type::high_resolution;

            throw invalid_value_exception("Invalid resolution!");
        }

        void apply_preset(const std::string& pid,
                          const std::vector<platform::stream_profile>& configuration,
                          rs2_advanced_mode_preset preset)
        {
            auto p = get_all();


            auto res = get_res_type(configuration.front().width, configuration.front().height);

            switch (preset) {
            case RS2_ADVANCED_MODE_PRESET_GENERIC_DEPTH:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:
                        generic_ds5_passive_small_seed_config(p);
                        break;
                    case vga_resolution:
                        generic_ds5_passive_vga_seed_config(p);
                        break;
                    case high_resolution:
                        generic_ds5_passive_full_seed_config(p);
                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case vga_resolution:
                        generic_ds5_active_vga_generic_depth(p);
                        break;
                    case high_resolution:
                        generic_ds5_active_full_generic_depth(p);
                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    throw invalid_value_exception("Invalid resolution!");
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_GENERIC_ACCURATE_DEPTH:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:
                        generic_ds5_passive_small_seed_config(p);
                        break;
                    case vga_resolution:
                        generic_ds5_passive_vga_seed_config(p);
                        break;
                    case high_resolution:
                        generic_ds5_passive_full_seed_config(p);
                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_GENERIC_DENSE_DEPTH:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:
                        generic_ds5_passive_small_seed_config(p);
                        break;
                    case vga_resolution:
                        generic_ds5_passive_vga_seed_config(p);
                        break;
                    case high_resolution:
                        generic_ds5_passive_full_seed_config(p);
                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_GENERIC_SUPER_DENSE_DEPTH:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:
                        generic_ds5_passive_small_seed_config(p);
                        break;
                    case vga_resolution:
                        generic_ds5_passive_vga_seed_config(p);
                        break;
                    case high_resolution:
                        generic_ds5_passive_full_seed_config(p);
                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_FLOOR_LOW:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        switch (res) {
                        case low_resolution:

                            break;
                        case vga_resolution:

                            break;
                        case high_resolution:

                            break;
                        default:
                            throw invalid_value_exception("Invalid resolution!");
                        }    break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_3D_BODY_SCAN:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_INDOOR:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_OUTDOOR:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_HAND:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_SHORT_RANGE:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            case RS2_ADVANCED_MODE_PRESET_BOX:
                if (pid == pid_to_str(ds::RS400_PID) || pid == pid_to_str(ds::RS400_MM_PID)) // Passive
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS410_PID) || pid == pid_to_str(ds::RS410_MM_PID)) // Active
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else if (pid == pid_to_str(ds::RS430_MM_PID)) // AWG
                {
                    switch (res) {
                    case low_resolution:

                        break;
                    case vga_resolution:

                        break;
                    case high_resolution:

                        break;
                    default:
                        throw invalid_value_exception("Invalid resolution!");
                    }
                }
                else
                    throw invalid_value_exception("Invalid pid!");
                break;
            default:
                throw invalid_value_exception("Invalid preset!");
            }

            set_all(p);
        }

        void get_depth_control_group(STDepthControlGroup* ptr, int mode = 0) const
        {
            *ptr = get<STDepthControlGroup>(advanced_mode_traits<STDepthControlGroup>::group, nullptr, mode);
        }

        void get_rsm(STRsm* ptr, int mode = 0) const
        {
            *ptr = get<STRsm>(advanced_mode_traits<STRsm>::group, nullptr, mode);
        }

        void get_rau_support_vector_control(STRauSupportVectorControl* ptr, int mode = 0) const
        {
            *ptr = get<STRauSupportVectorControl>(advanced_mode_traits<STRauSupportVectorControl>::group, nullptr, mode);
        }

        void get_color_control(STColorControl* ptr, int mode = 0) const
        {
            *ptr = get<STColorControl>(advanced_mode_traits<STColorControl>::group, nullptr, mode);
        }

        void get_rau_color_thresholds_control(STRauColorThresholdsControl* ptr, int mode = 0) const
        {
            *ptr = get<STRauColorThresholdsControl>(advanced_mode_traits<STRauColorThresholdsControl>::group, nullptr, mode);
        }

        void get_slo_color_thresholds_control(STSloColorThresholdsControl* ptr, int mode = 0) const
        {
            *ptr = get<STSloColorThresholdsControl>(advanced_mode_traits<STSloColorThresholdsControl>::group, nullptr, mode);
        }

        void get_slo_penalty_control(STSloPenaltyControl* ptr, int mode = 0) const
        {
            *ptr = get<STSloPenaltyControl>(advanced_mode_traits<STSloPenaltyControl>::group, nullptr, mode);
        }

        void get_hdad(STHdad* ptr, int mode = 0) const
        {
            *ptr = get<STHdad>(advanced_mode_traits<STHdad>::group, nullptr, mode);
        }

        void get_color_correction(STColorCorrection* ptr, int mode = 0) const
        {
            *ptr = get<STColorCorrection>(advanced_mode_traits<STColorCorrection>::group, nullptr, mode);
        }

        void get_depth_table_control(STDepthTableControl* ptr, int mode = 0) const
        {
            *ptr = get<STDepthTableControl>(advanced_mode_traits<STDepthTableControl>::group, nullptr, mode);
        }

        void get_ae_control(STAEControl* ptr, int mode = 0) const
        {
            *ptr = get<STAEControl>(advanced_mode_traits<STAEControl>::group, nullptr, mode);
        }

        void get_census_radius(STCensusRadius* ptr, int mode = 0) const
        {
            *ptr = get<STCensusRadius>(advanced_mode_traits<STCensusRadius>::group, nullptr, mode);
        }

        void set_depth_control_group(const STDepthControlGroup& val)
        {
            set(val, advanced_mode_traits<STDepthControlGroup>::group);
        }

        void set_rsm(const STRsm& val)
        {
            set(val, advanced_mode_traits<STRsm>::group);
        }

        void set_rau_support_vector_control(const STRauSupportVectorControl& val)
        {
            set(val, advanced_mode_traits<STRauSupportVectorControl>::group);
        }

        void set_color_control(const STColorControl& val)
        {
            set(val, advanced_mode_traits<STColorControl>::group);
        }

        void set_rau_color_thresholds_control(const STRauColorThresholdsControl& val)
        {
            set(val, advanced_mode_traits<STRauColorThresholdsControl>::group);
        }

        void set_slo_color_thresholds_control(const STSloColorThresholdsControl& val)
        {
            set(val, advanced_mode_traits<STSloColorThresholdsControl>::group);
        }

        void set_slo_penalty_control(const STSloPenaltyControl& val)
        {
            set(val, advanced_mode_traits<STSloPenaltyControl>::group);
        }

        void set_hdad(const STHdad& val)
        {
            set(val, advanced_mode_traits<STHdad>::group);
        }

        void set_color_correction(const STColorCorrection& val)
        {
            set(val, advanced_mode_traits<STColorCorrection>::group);
        }

        void set_depth_table_control(const STDepthTableControl& val)
        {
            set(val, advanced_mode_traits<STDepthTableControl>::group);
        }

        void set_ae_control(const STAEControl& val)
        {
            set(val, advanced_mode_traits<STAEControl>::group);
        }

        void set_census_radius(const STCensusRadius& val)
        {
            set(val, advanced_mode_traits<STCensusRadius>::group);
        }


    private:
        const std::map<float, std::string>& _description_per_value{{RS2_ADVANCED_MODE_PRESET_GENERIC_DEPTH,            "GENERIC_DEPTH"},
                                                                   {RS2_ADVANCED_MODE_PRESET_GENERIC_ACCURATE_DEPTH,   "GENERIC_ACCURATE_DEPTH"},
                                                                   {RS2_ADVANCED_MODE_PRESET_GENERIC_DENSE_DEPTH,      "GENERIC_DENSE_DEPTH"},
                                                                   {RS2_ADVANCED_MODE_PRESET_GENERIC_SUPER_DENSE_DEPTH,"GENERIC_SUPER_DENSE_DEPTH"},
                                                                   {RS2_ADVANCED_MODE_PRESET_FLOOR_LOW,                "FLOOR_LOW"},
                                                                   {RS2_ADVANCED_MODE_PRESET_3D_BODY_SCAN,             "3D_BODY_SCAN"},
                                                                   {RS2_ADVANCED_MODE_PRESET_INDOOR,                   "INDOOR"},
                                                                   {RS2_ADVANCED_MODE_PRESET_OUTDOOR,                  "OUTDOOR"},
                                                                   {RS2_ADVANCED_MODE_PRESET_HAND,                     "HAND"},
                                                                   {RS2_ADVANCED_MODE_PRESET_SHORT_RANGE,              "SHORT_RANGE"},
                                                                   {RS2_ADVANCED_MODE_PRESET_BOX,                      "BOX"}};

        std::shared_ptr<hw_monitor> _hw_monitor;
        uvc_sensor& _depth_sensor;
        lazy<bool> _enabled;

        static const uint16_t HW_MONITOR_COMMAND_SIZE = 1000;
        static const uint16_t HW_MONITOR_BUFFER_SIZE = 1024;

        preset get_all()
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

        void set_all(const preset& p)
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

        std::vector<uint8_t> send_recieve(const std::vector<uint8_t>& input) const
        {
            auto res = _hw_monitor->send(input);
            if (res.empty())
            {
                throw std::runtime_error("Advanced mode write failed!");
            }
            return res;
        }

        template<class T>
        void set(const T& strct, EtAdvancedModeRegGroup cmd) const
        {
            auto ptr = (uint8_t*)(&strct);
            std::vector<uint8_t> data(ptr, ptr + sizeof(T));

            assert_no_error(ds::fw_cmd::set_advanced,
                send_recieve(encode_command(ds::fw_cmd::set_advanced, static_cast<uint32_t>(cmd), 0, 0, 0, data)));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        template<class T>
        T get(EtAdvancedModeRegGroup cmd, T* ptr = static_cast<T*>(nullptr), int mode = 0) const
        {
            T res;
            auto data = assert_no_error(ds::fw_cmd::get_advanced,
                send_recieve(encode_command(ds::fw_cmd::get_advanced,
                static_cast<uint32_t>(cmd), mode)));
            if (data.size() < sizeof(T))
            {
                throw std::runtime_error("The camera returned invalid sized result!");
            }
            res = *reinterpret_cast<T*>(data.data());
            return res;
        }

        static uint32_t pack(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3)
        {
            return (c0 << 24) | (c1 << 16) | (c2 << 8) | c3;
        }

        static std::vector<uint8_t> assert_no_error(ds::fw_cmd opcode, const std::vector<uint8_t>& results)
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

        std::vector<uint8_t> encode_command(ds::fw_cmd opcode,
            uint32_t p1 = 0,
            uint32_t p2 = 0,
            uint32_t p3 = 0,
            uint32_t p4 = 0,
            std::vector<uint8_t> data = std::vector<uint8_t>()) const
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
    };


    class advanced_mode_preset_option : public option_base
    {
    public:
        advanced_mode_preset_option(ds5_advanced_mode_base& advanced, uvc_sensor& ep, const option_range& opt_range,
                                    const std::map<float, std::string>& description_per_value)
            : option_base(opt_range),
              _ep(ep),
              _description_per_value(description_per_value),
              _advanced(advanced)
        {}

        static rs2_advanced_mode_preset to_preset(float x)
        {
            return (rs2_advanced_mode_preset)((int)x);
        }

        void set(float value) override
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

        float query() const override
        {
            if (!_advanced.is_enabled())
                throw wrong_api_call_sequence_exception(to_string() << "set(advanced_mode_preset_option) failed! Device is not is Advanced-Mode.");

            return _last_preset;
        }

        bool is_enabled() const override
        {
            return _advanced.is_enabled();
        }

        const char* get_description() const override
        {
            return "Advanced-Mode Preset";
        }

        const char* get_value_description(float val) const override
        {
            try{
                return _description_per_value.at(val).c_str();
            }
            catch(std::out_of_range)
            {
                throw invalid_value_exception(to_string() << "advanced_mode_preset: get_value_description(...) failed! Description of value " << val << " is not found.");
            }
        }

    private:
        const std::map<float, std::string> _description_per_value;
        rs2_advanced_mode_preset _last_preset{};
        uvc_sensor& _ep;
        ds5_advanced_mode_base& _advanced;
    };
}
